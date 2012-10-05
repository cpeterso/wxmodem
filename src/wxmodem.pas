{$U-,C-,R-}
program WXMODEM;
{
Peter Boswell
}
Const
     VERSION = '4.0';

type
    bigstring        = string[80];    {general purpose}
    cset             = set of 0..127;
var
   transmit,
   pcjrmode              : boolean;
   exitfl,
   testfl,
   good,
   xtnd                  : boolean;
   displayfl   : boolean;
   a                     : byte;
   c,i                   : integer;
   fmin, fsec            : integer;
   xmofile               : bigstring;
   bytes                 : bigstring;
   rbytes                : real;
   ch, xmotype           : char;

{$C-,R-,U-,K-}
{$I WXMOPORT.INC}
{$I WXMOMISC.INC}
{$I WXMOWIND.INC}
{$I WXMOXFER.INC}
{$I WXMODIAL.INC}


begin
     displayfl := false;
     ScreenBase := $B000;                       {assume monochrome}
     MemW[ ScreenBase:0000 ]   := 69;
     if Mem[ ScreenBase:0000 ] <> 69 then
        ScreenBase := $B800;
     exitfl := true;
     setup;
     if displayfl then
     begin
         LowVideo;
         ClrScr;
         InitWindow(StatWin,1,1,80,2);
         InitWindow(TermWin,1,3,80,25);
         CurrentWin := TermWin;
         UsePermWindow(TermWin);
         status(1,'WXMODEM ver: ' + VERSION);
         status(2,'Initializing');
     end;
     init_port;
     term_ready(true);
     set_up_recv_buffer;
     remove_port;
     init_port;
     term_ready(true);
     set_up_recv_buffer;
     good := false;
     if displayfl then
     begin
          GotoXY(1,1);
          if carrier then
             status(2,'On-Line/Ready')
          else
             status(2,'Off-Line/Ready');
          status(3,'TCOMM Support');
     end;
     if scan(xtnd,a) then
     begin
          if xtnd then
          begin
             case a of
                  45   : {alt-X}
                         begin
                              if displayfl then
                                 OpenTemp(20,18,60,22,1);
                              writeln('   WXMODEM   ');
                              write('Do you really want to exit (Y/N)? ');
                              readln(ch);
                              if upcase(ch) = 'Y' then
                                 exitfl := TRUE;
                              if displayfl then
                                 CloseTemp
                         end;
                  48   : {alt-B}
                         break;
             end; {case}
          end     {if extended key}
     end; {if KeyPressed}
     if (exitfl = false) and carrier then
        case transmit of
            false : recv_wcp;
            true  : send_wcp;
        end;
     assign(cmfile,cmdfname);
     {$I-} Rewrite(cmfile) {I+};
     if IOresult = 0 then
     begin
         if good then
         begin
            rbytes := 128.0 * tblks;
            str(rbytes:7:0,bytes);
            writeln(cmfile,'0 '+xmofile+' '+bytes);
            {$I-} close(cmfile); {$I+}
            If IOresult <> 0 then
              writeln('WXTERM - error rewriting parm file');
         end;
      end;

     remove_port;
end.