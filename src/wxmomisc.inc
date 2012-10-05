{$U-,C-,R-}
function scan(var extend : boolean; var code : byte) : boolean;
{
 Uses BIOS service 16 to get a keystroke w/o echo. Sets 'extend' true
 for extended codes from PC-Clone keyboards, and returns ASCII/Scan code
 in 'code'.  Returns true if a character exists, false if none is in the
 buffer.
}
type
    regs = record
	  ax,bx,cx,dx,bp,si,di,ds,es,flags : integer;
    end;
var
   r : regs;
   c : integer;
begin
     code := 0;
     r.ax := 1 shl 8;	 {AH := 1}
     Intr($16,r);
     if r.flags and 64 <> 64 then
     begin
	r.ax := 0;
	Intr($16,r);	 {Get character and clear from buffer}
	code := lo(r.ax);
	scan := true;
	extend := false;
	if  code = 0 then
	begin
	   extend := true;
	   code := hi(r.ax)
	end;
     end
     else
	scan := false;
end;

function exists(fname :  bigstring) : boolean;
var
   f : file;
begin
     assign(f, fname);
     {$I-}
     reset(f);
     {$I+}
     if IOresult = 0 then
	begin
	     exists := true;
	     close(f)
	end
     else
	exists := false
end;

procedure supcase(var s);
var
   ss : bigstring absolute s;
   i : integer;
begin
     for i := 1 to length(ss) do
	 ss[i] := upcase(ss[i])
end;