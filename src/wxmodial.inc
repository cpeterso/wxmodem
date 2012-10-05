{$U-,C-,R-,K-}
Var
  cmfile      : text;
  cmdfname    : bigstring;

procedure setup;
{initialize most stuff -

 Command Line:  WXMODEM filename [w/c/x] [pcjr] [test]

        filename -      name of control file
        [w/c/x]  -      default is w
                        w - windowed xmodem
                        c - crc xmodem
                        x - checksum xmodem
        [pcjr]   -      if present, xoff pause during disk i/o on receive

        [test]   -      if present, no user messages are sent through com port

 Control file:

  [s/r] filename [p] [baud] [parity] [display] [color]

}
Var
  n, j, k, l  : integer;
  result      : integer;
  paramstring : bigstring;
  cspeed      : bigstring;
  parity_str  : bigstring;
  com_str     : bigstring;
Label
  99;
begin
     exitfl := true;                     {assume an error}
     n := ParamCount;                    { Parameter count }
     if n < 1 then                       { Must be atleast one parm }
     begin
         writeln('WXMODEM - No filename present on command line!');
         goto 99;
     end;
                                         { Process parms }
     testfl := false;
     xmotype := 'W';
     cmdfname  := ParamStr(1);           { command filename }
     if n > 1 then
     begin
         for i := 2 to n do
         begin
             paramstring := ParamStr(i);
             if (length(paramstring) = 1) and (i = 2) then
             begin
                 case UpCase(paramstring[1]) of
                    'W' : xmotype := 'W';
                    'C' : xmotype := 'C';
                    'X' : xmotype := 'X';
                    else begin
                            writeln('WXMODEM - Xmodem type must be W/C/X');
                            goto 99;
                         end;
                 end;         { of case }
             end
             else
             begin            {either length > or i > 2}
                 for j := 1 to length(paramstring) do
                     paramstring[j] := UpCase(paramstring[j]);
                 if paramstring = 'PCJR'   then pcjrmode := true;
                 if paramstring = 'NOPCJR' then pcjrmode := false;
                 if paramstring = 'TEST' then
                 begin
                     writeln('Test mode!!!!');
                     testfl := true;
                 end;
             end;
         end;                  { of for loop}
     end;
     if exists(cmdfname) then
     begin

        Assign(cmfile,cmdfname);
        {I-} Reset(cmfile); {I+}
        If IOresult <> 0 then
        begin
           writeln('WXMODEM - Unable to open command file');
           goto 99;
        end;
        read(cmfile,paramstring);     {get the stuff}
        i := 1;                       {string pointer}
        while (i <= length(paramstring)) and
              (paramstring[i] = ' ') do
               i := i + 1;
        case upcase(paramstring[i]) of
            'S'  : transmit := true;
            'R'  : transmit := false;
           else
           begin
              writeln('WXMODEM - Expecting S/R');
              goto 99;
           end;
        end;
        i := i + 1;
        while (i <= length(paramstring)) and
              (paramstring[i] = ' ') do
               i := i + 1;
        j := i;
        while (j <= length(paramstring)) and
              (paramstring[j] <> ' ') do
               j := j + 1;
        if j <= i then
        begin
            writeln('WXMODEM - xmodem filename missing');
            goto 99;
        end;
        k := j - i;
        for l := 1 to k do
        begin
            xmofile[l] := paramstring[i];
            i := i + 1;
        end;
        xmofile[0] := chr(k);
        i := j;
        while (i <= length(paramstring)) and
              (paramstring[i] = ' ') do
               i := i + 1;
        j := i;
        while (j <= length(paramstring)) and
              (paramstring[j] <> ' ') do
               j := j + 1;
        if j <= i then
        begin
            writeln('WXMODEM - COM Port parameter missing',j:4,i:4);
            goto 99;
        end;
        k := j - i;
        for l := 1 to k do
        begin
            com_str[l] := paramstring[i];
            i := i + 1;
        end;
        com_str[0] := chr(k);
        if (com_str = 'COM1') or (com_str = 'COM2')  then
        begin
           case com_str[4] of
             '1' :  Cport := com1;
             '2' :  Cport := com2;
              else
              begin
                 writeln('WXMODEM - COM'+com_str+' not supported, only COM1/COM2');
                 goto 99;
              end;
           end;     {of case}
        end
        else
        begin
           writeln('WXMODEM - Unable to decifer COM port');
           goto 99;
        end;
        speed := 1200;
        i := j;
        while (i <= length(paramstring)) and
              (paramstring[i] = ' ') do
               i := i + 1;
        j := i;
        while (j <= length(paramstring)) and
              (paramstring[j] <> ' ') do
             j := j + 1;
        if j > i then
        begin
            for k := 1 to j - i do
            begin
               if (paramstring[i] >= '0') and (paramstring[i] <= '9') then
                  cspeed[k] := paramstring[i];
               i := i + 1;
            end;
            cspeed[0] := chr(k);
            val(cspeed,speed,result);
            if result <> 0 then
            begin
               writeln('WXMODEM - Speed not recognized');
               speed := 1200;
            end;
        end;
        i := j;
        while (i <= length(paramstring)) and
              (paramstring[i] = ' ') do
               i := i + 1;
        j := i;
        while (j <= length(paramstring)) and
              (paramstring[j] <> ' ') do
               j := j + 1;
        if j <= i then
        begin
            writeln('WXMODEM - parity missing from file');
            goto 99;
        end;
        case upcase(paramstring[i]) of
           'E'  : begin
                      parity := even;
                      dbits  := 7;
                  end;
           'N'  : begin
                      parity := none;
                      dbits  := 8;
                  end;
           else
           begin
               writeln('WXMODEM - Parity must be Even or None');
               goto 99;
           end;
        end;
        stop_bits := 1;   {that's the only choice!}
        i := i + 1;
        while (i <= length(paramstring)) and
              (paramstring[i] = ' ') do
               i := i + 1;
        if paramstring[i] = '0' then
            displayfl := false
        else
        if paramstring[i] = '1' then
            displayfl := true;
        exitfl := false;     {good }
     end
     else
     begin
        writeln('WXMODEM - command file does not exist');
     end;
99:
     close(cmfile);
     if exitfl = true then
     begin
        writeln(paramstring);
        delay(10000);
        displayfl := false;
     end;
  end;