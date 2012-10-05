{$U-,C-,R-,K-}
{ A set of routines for text window manipulation
  By Bela Lubkin
  Borland International Technical Support
  1/10/85
  2/20/85 Bug fix: DisposeWindow left a bunch of junk on the heap, causing
    uncontrolled growth!
  (For PC-DOS Turbo Pascal version 2 or greater)
}
Type
  XTCoord=1..80;   { X Text coordinate }
  YTCoord=1..25;   { Y Text coordinate }
  XTCoord0=0..80;  { X Text coordinate + 0 for nothing }
  YTCoord0=0..25;  { Y Text coordinate + 0 for nothing }
  WindowRec=Record
              XSize: XTCoord;
              YSize: YTCoord;
              XPosn: XTCoord;
              YPosn: YTCoord;
              Contents: Array [0..1999] Of Integer;
            End;
  WindowPtr=^WindowRec;

Var
  WindowXLo: XTCoord;
  WindowYLo: YTCoord;
  WindowXHi: XTCoord;
  WindowYHi: YTCoord;
  ScreenBase: Integer;

Procedure TurboWindow(XL: XTCoord; YL: YTCoord; XH: XTCoord; YH: YTCoord);
{ This procedure provides an entry to Turbo's built in Window procedure }
  Begin
    Window(XL,YL,XH,YH);
  End;

Procedure Window(XL: XTCoord; YL: YTCoord; XH: XTCoord; YH: YTCoord);
{ This procedure replaces Turbo's built in Window procedure.  It calls the
  original Window procedure, and also keeps track of the window boundaries. }

  Begin
    TurboWindow(XL,YL,XH,YH);
    WindowXLo:=XL;
    WindowYLo:=YL;
    WindowXHi:=XH;
    WindowYHi:=YH;
  End;

Function SaveWindow(XLow: XTCoord; YLow: YTCoord;
                    XHigh: XTCoord; YHigh:YTCoord): WindowPtr;
{ Allocate a WindowRec of the precise size needed to save the window, then
  fill it with the text that is in the window XLow..XHigh, YLow..YHigh.
  Return a pointer to this WindowRec. }

  Var
    SW: WindowPtr;
    I: Integer;
    XS: XTCoord;
    YS: YTCoord;

  Begin
    XS:=XHigh-XLow+1;
    YS:=YHigh-YLow+1;
    GetMem(SW,2*XS*YS + 4);
    { Allocate 2 bytes for each screen position, + 4 for size and position }
    With SW^ Do
     Begin
      XSize:=XS;
      YSize:=YS;
      XPosn:=XLow;
      YPosn:=YLow;
      For I:=0 To YSize-1 Do
        Move(Mem[ScreenBase:((YPosn+I-1)*80+XPosn-1) Shl 1],
             Contents[I*XSize],XSize Shl 1);
      { For each line of the window,
          Move XSize*2 bytes (1 for char, 1 for attribute) into the Contents
               array.  Leave no holes in the array. }
     End;
    SaveWindow:=SW;
  End;

Function SaveCurrentWindow: WindowPtr;
  Begin
    SaveCurrentWindow:=SaveWindow(WindowXLo,WindowYLo,WindowXHi,WindowYHi);
  End;

Procedure RestoreWindow(WP: WindowPtr; XPos: XTCoord0; YPos: YTCoord0);
{ Given a pointer to a WindowRec, restore the contents of the window.  If
  XPos or YPos is 0, use the XPosn or YPosn that the window was originally
  saved with.  If either is nonzero, use it.  Thus a window can be restored
  exactly with  RestoreWindow(wp,0,0);  or its upper left corner can be
  placed at (2,3) with  RestoreWindow(wp,2,3); }

  Var
    I: Integer;

  Begin
    With WP^ Do
     Begin
      If XPos=0 Then XPos:=XPosn;
      If YPos=0 Then YPos:=YPosn;
      For I:=0 To YSize-1 Do
        Move(Contents[I*XSize],
             Mem[ScreenBase:2*((YPos+I-1)*80+XPos-1)],XSize*2);
      { For each line of the window,
          Move XSize*2 bytes (1 for char, 1 for attribute) from the Contents
               array onto the screen. }
     End;
  End;

Procedure DisposeWindow(Var WP: WindowPtr);
{ Dispose of a WindowPtr.  The built in procedure Dispose cannot be used,
  because it will deallocate SizeOf(WindowRec) bytes, even though less may
  have been allocated. }

  Begin
    With WP^ Do FreeMem(WP,2*XSize*YSize+4);
    WP:=Nil;
  End;

Procedure DRestoreWindow(Var WP: WindowPtr; XPos: XTCoord0; YPos: YTCoord0);
{ Restore the contents of a window, then dispose of the saved image }

  Begin
    RestoreWindow(WP, XPos, YPos);
    DisposeWindow(WP);
  End;

Procedure DRestoreCurrentWindow(Var WP: WindowPtr;
                                XPos: XTCoord0; YPos: YTCoord0);
{ Restore the contents of a window, set the current window to fit the restored
  window, and dispose of the saved image.  A similar procedure
  RestoreCurrentWindow could be written by changing DRestoreWindow to
  RestoreWindow in the last line of the procedure, but I have assumed that
  when you select a window area, you are going to modify it, and not want the
  old image }

  Begin
    With WP^ Do
     Begin
      If XPos=0 Then XPos:=XPosn;
      If YPos=0 Then YPos:=YPosn;
      Window(XPos,YPos,XPos+XSize-1,YPos+YSize-1);
     End;
    DRestoreWindow(WP, XPos, YPos);
  End;

{****** My interface - S. Murphy ******}

type
    WindowParms = record
        col1, col2,
        row1, row2 : integer;          {corner co-ordinates}
        frame : 0..2;                  {border type}
        CursorX, CursorY : integer;          {cursor position}
    end;

    WindowDescriptor = ^WindowParms;
Var
   StatWin, TermWin,
   CurrentWin, border    : WindowDescriptor;
   TempWin               : WindowPtr;
   StackedPage           : WindowPtr;

procedure UsePermWindow(var w : WindowDescriptor);
begin
     with CurrentWin^ do
     begin
          CursorX := WhereX;
          CursorY := WhereY
     end;
     CurrentWin := w;
     with w^ do
     begin
          window(col1,row1,col2,row2);
          GotoXY(CursorX, CursorY)
     end
end;

procedure Status(slot :integer; msg : bigstring);
var
   i : integer;
begin
     if not displayfl then
        exit;
     UsePermWindow(StatWin);
     GotoXY(20*slot+1,1);
     if slot < 3 then
         write('                    ')
     else
         write('                   ');
     GotoXY(20*slot+1,1);
     write(msg);
     UsePermWindow(TermWin)
end;


procedure InitWindow(var w : WindowDescriptor;
                         x1, y1, x2, y2 : integer);
begin
     new(w);
     with w^ do
     begin
          col1 := x1;
          col2 := x2;
          row1 := y1;
          row2 := y2;
          CursorX := 1;
          CursorY :=1
     end
end;

procedure DrawBox(col1, row1, col2, row2, frame : integer);
type
    cvec6 = array[1..6] of char;
    cptr = ^cvec6;
const
     V1 = #179;   UR1 = #191;   UL1 = #218;
     V2 = #186;   UR2 = #187;   UL2 = #201;
     H1 = #196;   LR1 = #217;   LL1 = #192;
     H2 = #205;   LR2 = #188;   LL2 = #200;

     SFRAME : cvec6 = (UL1,H1,UR1,V1,LL1,LR1);
     DFRAME : cvec6 = (UL2,H2,UR2,V2,LL2,LR2);

var
   framedef : cptr;
   i,j : integer;
begin
     if frame <> 0 then
     begin
          case frame of
             1 : framedef := ptr(seg(SFRAME),ofs(SFRAME));
             2 : framedef := ptr(seg(DFRAME),ofs(DFRAME))
          end;
          GotoXY(col1, row1);
          write(framedef^[1]);
          for i := col1 + 1 to col2 - 1 do
              write(framedef^[2]);
          write(framedef^[3]);
          for i := row1 + 1 to row2 - 1 do
          begin
                GotoXY(col1, i);
                write(framedef^[4]);
                GotoXY(col2, i);
                write(framedef^[4])
          end;
          GotoXY(col1, row2);
          write(framedef^[5]);
          for i := col1 + 1 to col2 - 1 do
              write(framedef^[2]);
          write(framedef^[6])
    end
end;

Procedure OpenTemp(x1,y1,x2,y2,border : integer);
begin
     if not displayfl then
        exit;
     with CurrentWin^ do
     begin
       CursorX := WhereX;
       CursorY := WhereY;
       TempWin := SaveWindow(col1,row1,col2,row2)
     end;
     DrawBox(x1,y1,x2,y2,border);
     TurboWindow(x1+1, y1+3, x2-1, y2+1);
     ClrScr;
     GotoXY(1,1)
end;

Procedure CloseTemp;
begin
     if not displayfl then
        exit;
     DRestoreWindow(TempWin,0,0);
     with CurrentWin^ do
     begin
        TurboWindow(col1,row1,col2,row2);
        GotoXY(CursorX,CursorY)
     end
end;

procedure PushPage;
const
     MEMNEEDED = 3696; {memory overhead to store a page}
Var
   c : char;
begin
     if MemAvail >= MEMNEEDED then
     begin
          OpenTemp(20,5,75,10,2);
          write('Save this screen? (Y/N; default N) ');
          readln(c);
          CloseTemp;
          if c in  ['Y','y'] then
               StackedPage := SaveWindow(1,3,80,25)
     end
     else begin
          OpenTemp(30,5,70,10,2);
          writeln('Out of Memory: Can''t save page.');
          write('Type <cr> to continue.');
          readln
     end
end;

procedure PopPage;
begin
     if StackedPage <> NIL then
          DRestoreWindow(StackedPage,0,0)
end;