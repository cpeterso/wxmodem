{$U-,C-,R-,K-}
  {   - modified to add CRC xmodem, wxmodem 7/86 - 10/86
Peter Boswell
ADI
Suite 650
350 N. Clark St.
Chicago, Il 60610
People/Link: Topper
Compuserve : 72247,3671
  }
const
     SOH = 1;                          {Start Of Header}
     EOT = 4;                          {End Of Transmission}
     ACK = 6;                          {ACKnowledge}
     DLE = $10;                        {Data Link Escape}
     XON = $11;                        {X-On}
     XOFF = $13;                       {X-Off}
     NAK = $15;                        {Negative AcKnowledge}
     SYN = $16;                        {Synchronize}
     CAN = $18;                        {CANcel}
     CHARC = $43;                      {C = CRC Xmodem}
     CHARW = $57;                      {W = WXmodem}
     MAXERRS = 10;                     {Maximum allowed errors}
     L = 0;
     H = 1;
     Buflen  = 128;                    {Disk I/O buffer length}
     Bufnum = 64;                      {Disk I/O buffer count}
     Maxwindow = 4;                    {Wxmodem window size}
                                       {CRC byte translation table}
     Crctab: array[0..255] of Integer =
     (0, 4129, 8258, 12387, 16516, 20645, 24774, 28903,
      -32504,-28375,-24246,-20117,-15988,-11859,-7730,-3601,
      4657, 528, 12915, 8786, 21173, 17044, 29431, 25302,
      -27847,-31976,-19589,-23718,-11331,-15460,-3073,-7202,
      9314, 13379, 1056, 5121, 25830, 29895, 17572, 21637,
      -23190,-19125,-31448,-27383,-6674,-2609,-14932,-10867,
      13907, 9842, 5649, 1584, 30423, 26358, 22165, 18100,
      -18597,-22662,-26855,-30920,-2081,-6146,-10339,-14404,
      18628, 22757, 26758, 30887, 2112, 6241, 10242, 14371,
      -13876,-9747,-5746,-1617,-30392,-26263,-22262,-18133,
      23285, 19156, 31415, 27286, 6769, 2640, 14899, 10770,
      -9219,-13348,-1089,-5218,-25735,-29864,-17605,-21734,
      27814, 31879, 19684, 23749, 11298, 15363, 3168, 7233,
      -4690,-625,-12820,-8755,-21206,-17141,-29336,-25271,
      32407, 28342, 24277, 20212, 15891, 11826, 7761, 3696,
      -97,-4162,-8227,-12292,-16613,-20678,-24743,-28808,
      -28280,-32343,-20022,-24085,-12020,-16083,-3762,-7825,
      4224, 161, 12482, 8419, 20484, 16421, 28742, 24679,
      -31815,-27752,-23557,-19494,-15555,-11492,-7297,-3234,
      689, 4752, 8947, 13010, 16949, 21012, 25207, 29270,
      -18966,-23093,-27224,-31351,-2706,-6833,-10964,-15091,
      13538, 9411, 5280, 1153, 29798, 25671, 21540, 17413,
      -22565,-18438,-30823,-26696,-6305,-2178,-14563,-10436,
      9939, 14066, 1681, 5808, 26199, 30326, 17941, 22068,
      -9908,-13971,-1778,-5841,-26168,-30231,-18038,-22101,
      22596, 18533, 30726, 26663, 6336, 2273, 14466, 10403,
      -13443,-9380,-5313,-1250,-29703,-25640,-21573,-17510,
      19061, 23124, 27191, 31254, 2801, 6864, 10931, 14994,
      -722,-4849,-8852,-12979,-16982,-21109,-25112,-29239,
      31782, 27655, 23652, 19525, 15522, 11395, 7392, 3265,
      -4321,-194,-12451,-8324,-20581,-16454,-28711,-24584,
      28183, 32310, 20053, 24180, 11923, 16050, 3793, 7920);

{*** variables used as globals in this source segment
     (actually global to whole  source) ***}
var
   checksum     : integer;
   tblks        : integer;
   fname        : bigstring;
   response     : string[1];
   crcval,db,sb : integer;
   packetln     : integer;            {128 + Checksum or 128 + CRC}
   p            : parity_set;
   dbuffer      : array[1..Bufnum,1..Buflen] of byte;
   dcount       : integer;
   Wxmode       : boolean;
   Crcmode      : boolean;
   Openflag     : boolean;
   Windowfl     : boolean;


procedure updcrc(a : byte);
begin
  {
     crcval := Crctab[hi(crcval) xor a] xor (lo(crcval) shl 8);
  }
     inline(

        $A1/crcval/       {mov ax,crcval     AX <- crcval}
        $89/$C2/          {mov dx,ax         DX <- crcval}
        $88/$E0/          {mov al,ah         (AX) crcval >> 8}
        $B4/$00/          {mov ah,0 }
        $36/              {ss:}
        $8B/$8E/a/        {mov cx,[bp+a]     CX <- a}
        $31/$C8/          {xor ax,cx         AX <- (crcval >> 8) xor a}
        $D1/$E0/          {shl ax,1          AX <- AX * 2  (word index)}
        $BB/crctab/       {mov bx,offset crctab   BX <- addr(crctab)}
        $01/$C3/          {add bx,ax         BX <- addr(crctab)+((crcval>>8)xor a)*2 }
        $2E/              {cs:}
        $8B/07/           {mov ax,[bx]       AX <- contents of crctab}
        $88/$D6/          {mov dh,dl         (DX) crcval << 8}
        $B2/$00/          {mov dl,00}
        $31/$D0/          {xor ax,dx         AX <- contents of crctab xor crcval << 8}
        $A3/crcval        {mov crcval,ax     crcval <- AX}

          );
end;
{ Xmodem transmit window routine
  Peter Boswell, July 1986       }

procedure txwindow(opt : integer; in_string : bigstring);

begin
   if not displayfl then
      exit;
   case opt of
       1  :     begin                           {initialize}
                   OpenTemp(36,3,78,18,2);
                   Windowfl := true;
                   Clrscr;
                   GotoXY(10,1);
                   write('File - ',in_string);
                   GotoXY(10,2);
                   write('Mode -');
                   GotoXY(4,3);
                   write('Total time -');
                   GotoXY(2,4);
                   write('Total Blocks -');
                   GotoXY(10,5);
                   write('Sent -');
                   GotoXY(9,6);
                   write('ACK''d -');
                   GotoXY(6,7);
                   write('Last NAK -');
                   GotoXY(9,8);
                   write('X-Off - No');
                   GotoXY(8,9);
                   write('Window - 0');
                   GotoXY(4,11);
                   write('Last Error -');
                   GotoXY(8,10);
                   write('Errors -');
                end;
       2..11  : begin
                   GotoXY(17,opt);
                   ClrEol;
                   write(in_string);
                end;
       12     : begin
                   GotoXY(3,12);
                   ClrEol;
                   write(in_string);
                end;
       99     : begin
                   if windowfl then
                      CloseTemp;
                end;

   end;                                         {case}
end;
{ Xmodem receive window routine
  Peter Boswell, October 1986       }

procedure trwindow(opt : integer; in_string : bigstring);

begin
   if not displayfl then
       exit;
   case opt of
       1  :     begin                           {initialize}
                   windowfl := true;
                   OpenTemp(36,3,78,13,2);
                   Clrscr;
                   GotoXY(10,1);
                   write('File - ',in_string);
                   GotoXY(10,2);
                   write('Mode -');
                   GotoXY(6,3);
                   write('Received -');
                   GotoXY(6,4);
                   write('Last NAK -');
                   GotoXY(4,5);
                   write('Last Error -');
                   GotoXY(8,6);
                   write('Errors -');
                end;
       2..6   : begin
                   GotoXY(17,opt);
                   ClrEol;
                   write(in_string);
                end;
       8      : begin
                   GotoXY(3,8);
                   ClrEol;
                   write(in_string);
                end;
       99     : begin
                   if windowfl then
                      CloseTemp;
                end;
   end;                                         {case}
end;

{
  This routine deletes all DLE characters and XOR's the following character
  with 64.  If a SYN character is found then -2 is returned.
    }
function dlecgetc(Tlimit : integer) : integer;
var
savecgetc : integer;
begin
     if wxmode then
     begin
          savecgetc := cgetc(Tlimit);
          if savecgetc = SYN then
             savecgetc := -2
          else
          if savecgetc = DLE then
          begin
               savecgetc := cgetc(Tlimit);
               if savecgetc >= 0 then savecgetc := savecgetc XOR 64;
          end;
          dlecgetc := savecgetc;
     end
     else
     dlecgetc := cgetc(Tlimit);
end;

procedure purge;
begin
     while dlecgetc(1) >= 0 do
                     ;
end;

procedure SaveCommStatus;
begin
      p := parity;
      db := dbits;
      sb := stop_bits;
      dbits        := 8;
      parity       := none;
      stop_bits    := 1;
      update_uart
end;


procedure recv_wcp;
{receive a file using Ward Christensen's checksum protocol}
label
     99;
var
   j, firstchar, sectnum, sectcurr, prevchar, lignore, blkcnt,
   toterr, errors, sectcomp, bufcurr, bresult : integer;
   Xtrace, EotFlag, ErrorFlag, Extend : boolean;
   UserKey : byte;
   blkfile : file;
   statstr : bigstring;
   trfile                     : text;
begin
     status(2, 'RECV XMODEM');
     windowfl  := False;
     EotFlag   := False;
     Xtrace    := False;
     Openflag  := False;
     Bufcurr   := 1;
     SaveCommStatus;
     fname     := xmofile;
     Assign(blkfile,fname);
     {$I-} Rewrite(blkfile); {$I+}
     ErrorFlag := (IOresult <> 0);
     if ErrorFlag then
     begin
        writeln(#13,#10,'WXMODEM --- cannot open receive file');
        goto 99;
     end
     else
        openflag := True;

     trwindow(1, fname);
     blkcnt := 0;
     sectnum := 0;
     errors := 0;
     toterr := 0;
{    assign(trfile,'trace');}
{    rewrite(trfile);}
     Crcmode  := true;                          {Assume CRC versus Checksum}
     Packetln := 130;                           {128 byte data + 2 byte CRC}
     Wxmode   := true;                          {Assume Wxmodem}
     Lignore  := 0;                             {ignore packets after error}
     i:=0;                                      {Try for Wxmodem 3 times}
     purge;
     if xmotype = 'W' then
     begin
          trwindow(8,'Trying Wxmodem');
          repeat
               send(ord('W'));
               firstchar := cgetc(6);               {6 seconds each}
               if scan(Extend, UserKey) then
                    if UserKey = CAN then goto 99;
               i := i + 1;
          until (firstchar=SYN) or (firstchar=CAN) or (i=7);
          if firstchar=CAN then goto 99;
          if firstchar <> SYN then
               xmotype := 'C';
     end;
     if xmotype = 'C' then
     begin
          trwindow(8,'Trying CRC Xmodem');
          wxmode := false;
          i:=0;                                 {Try CRC xmodem 3 times}
          repeat
               send(ord('C'));
               firstchar := cgetc(4);           {4 seconds each}
               if scan(Extend,UserKey) then
                    if UserKey = CAN then goto 99;
               i := i + 1;
          until (firstchar=SOH) or (firstchar=CAN) or (i=3);
          if firstchar = CAN then goto 99;
          if firstchar <> SOH then
               xmotype := 'X';
     end;
     if xmotype = 'X' then
     begin
          trwindow(5,'Trying Checksum Xmodem');
          wxmode   := false;
          Crcmode  := false;
          Packetln := 129;                 {128 bytes + 1 byte Checksum}
          i:=0;                            {Try Checksum xmodem 4 times}
          repeat
               send(NAK);
               firstchar := cgetc(10);     {10 seconds each}
               if scan(Extend,UserKey) then
                    if UserKey = CAN then goto 99;
               i := i + 1;
          until (firstchar=SOH) or (firstchar=CAN) or (i=4);
     end;                                  {Checksum}
        { firstchar contains the first character and Wxmode and Crcmode
          indicate the type of Xmodem }

     If wxmode then
         trwindow(2,'WXmodem');
     If not wxmode and crcmode then
         trwindow(2,'CRC Xmodem');
     if not wxmode and not crcmode then
         trwindow(2,'Checksum Xmodem');
     trwindow(8,'Press ^X to quit');
     prevchar := firstchar;                     {save the firstchar}
     while (EotFlag = false) and (Errors < MAXERRS) do
     begin                                      {locate start of packet}
       if (firstchar=SOH) and
          ((Wxmode and (prevchar=SYN)) or (not Wxmode)) then
       begin                                    {process packet}
          prevchar := -1;
          firstchar := -1;
          sectcurr := dlecgetc(15);
{         writeln(trfile,'sectcurr=',sectcurr:4);}
          sectcomp := dlecgetc(15);
          if sectcurr = (sectcomp xor 255) then
          begin                                 {sequence versus compl 1ood}
               if sectcurr = ((sectnum + 1) and 255) then
               begin                            {in sequence}
                    crcval   := 0;
                    checksum := 0;
                    j        := 1;
                    repeat
                         firstchar := dlecgetc(15);
                         if firstchar >= 0 then
                         begin
                              if j < 129 then
                                 dbuffer[bufcurr,j] := firstchar;
                              if Crcmode then updcrc(firstchar)
                              else checksum := (checksum and 255) + firstchar;
                              j := j + 1;
                         end;
                    until (j > Packetln) or (firstchar < 0);
                    if j > Packetln then        {good packet length}
                    begin
                         if (Crcmode and (crcval=0) or
                         (not Crcmode and ((checksum shr 1) = firstchar)))
                         then
                         begin                  {good crc/checksum}
                              firstchar := -1;  {make sure this byte not used
                                                 for start of packet }                errors  := 0;
                              sectnum := sectcurr;
                              blkcnt  := blkcnt + 1;
                              send(ACK);
                              str(blkcnt:4,statstr);
                              trwindow(3,statstr);
                              if Wxmode then send(sectcurr and 3);
{                             write(trfile,' ACK ');}
{                             if Wxmode then write(trfile,(sectcurr and 3):1);}
                              if errors <> 0 then
                              begin
                                 trwindow(6,'0');
                                 trwindow(5,' ');
                                 errors := 0;
                              end;
                              bufcurr := bufcurr + 1;
                              if bufcurr > bufnum then
                              begin             {Disk write routine}
                                   bufcurr := 1;
                                   if wxmode and pcjrmode then
                                   begin               {if unable to overlap
                                                        disk i/o and comm i/o.}
                                        send(XOFF);    {stop transmitter}
                                        delay(250);    {give it a chance}
                                   end;
                                   BlockWrite(blkfile,dbuffer,bufnum,bresult);
                                   if wxmode and pcjrmode then
                                   begin
                                        flush(blkfile); {complete all i/o}
                                        send(XON);      {restart transmitter}
                                   end;
                                   if bresult <> bufnum then
                                   begin
                                        trwindow(8,'Disk write error');
                                        goto 99;
                                   end;
                              end;              {End of disk write routine}
                         end                    {good crc/checksum}
                         else
                         begin                  {bad crc/checksum}
                              trwindow(5,'CRC/Checksum error');
                              str((blkcnt+1):6,statstr);
                              trwindow(4,statstr);
                              errors := errors + 1;
                              toterr := toterr + 1;
                              str(errors:3,statstr);
                              trwindow(6,statstr);
                              purge;  {clear any garbage coming in}
                              send(NAK);
                              if wxmode then
                              begin
                                   send(sectcurr and 3);
                                   lignore := maxwindow;
                              end;
{                             write(trfile,' NAK CRC ',(sectcurr and 3):1);}
                         end;                   {bad crc/checsum}
                    end                         {good packet length}
                    else
                    begin                       {bad packet length}
                         trwindow(5,'Short block error');
                         str((blkcnt+1):6,statstr);
                         trwindow(4,statstr);
                         errors := errors + 1;
                         str(errors:3,statstr);
                         trwindow(6,statstr);
                         purge;   {clear any garbage}
                         send(NAK);
                         if wxmode then
                         begin
                              send(sectcurr and 3);
                              lignore := maxwindow;
                         end;
                         purge;   {clear any garbage}
{                        write(trfile,' NAK SHORT ',(sectcurr and 3):1);}
                    end;                        {bad packet length}
               end                              {good block sequence number}
               else
               begin                            {invalid sequence number}
                    if lignore <= 0 then        {are we ignoring packets?}
                    begin
                         toterr := toterr + 1;
                         trwindow(5,'Out of sequence');
                         str((blkcnt+1):6,statstr);
                         trwindow(4,statstr);
                         errors := errors + 1;
                         str(errors:3,statstr);
                         trwindow(6,statstr);
                         purge;   {clear any garbage coming in}
                         send(NAK);
                         if wxmode then
                         begin
                              send((sectnum+1) and 3);
                              lignore := Maxwindow;
                         end;
                         purge;   {clear any garbage coming in}
{                        write(trfile,' NAK SEQ ',((sectnum+1) and 3):1);}
                    end
                    else lignore := lignore -1
               end;                             {invalid sequence number}
          end                                   {valid complement}
          else
          begin                                 {invalid complement}
               toterr := toterr + 1;
               trwindow(5,'Sequence complement error');
               str((blkcnt+1):6,statstr);
               trwindow(4,statstr);
               errors := errors + 1;
               str(errors:3,statstr);
               trwindow(6,statstr);
               purge;    {clear any garbage comming in}
               send(NAK);
               if wxmode then
               begin
                    send((sectnum+1) and 3);
                    lignore := Maxwindow;
               end;
               purge;    {clear any garbage comming in}
{              write(trfile,' NAK CMP ',((sectnum + 1) and 3):1);}
          end;                                  {invalid complement}
       end                                      {process packet}
       else                                     {not start of packet}
       begin
            case prevchar of
              EOT:   begin
                          if firstchar=EOT then
                          begin
                               EotFlag := True;
                               send(ACK);
                          end;
                     end;
              CAN:   begin
                          if firstchar=CAN then
                          goto 99;
                     end;
            end;                                {Of case}
            if not EotFlag then
            begin
                 if firstchar=EOT then
                 begin
                      send(NAK);  {first EOT received}
                      trwindow(5,' First EOT received');
                 end;
                 prevchar := firstchar;
                 firstchar := cgetc(15);        {start of packet!!!!}
                 if firstchar=-1 then
                 begin
                      if (prevchar=CAN) or (prevchar=EOT) then
                         firstchar := prevchar  {assume two have been received}
                      else
                      begin
                           trwindow(5,'Timeout on start of packet');
                           str((blkcnt+1):6,statstr);
                           trwindow(4,statstr);
                           errors := errors + 1;
                           str(errors:3,statstr);
                           trwindow(6,statstr);
                           send(XON);
                           toterr := toterr + 1;
                           send(NAK);
                           if wxmode then
                           begin
                                send((sectnum+1) and 3);
                                lignore := Maxwindow;
                           end;
{                          write(trfile,' NAK TIM ',((sectnum+1) and 3):1);}
                      end;
                 end;                           {Timeout at start of packet}
                 if scan(Extend,UserKey) then
                      if UserKey = CAN then goto 99;
            end;                                {end of not EotFlag}
         end;                                   {not start of packet}
     end;                                       {xmodem loop}
           {If there are any xmodem packets left in dbuffer, we had best
            write them out}

     If EotFlag and (bufcurr>1) then
     begin
          bufcurr := bufcurr - 1;
          trwindow(8,'Writing final blocks');
          if wxmode and pcjrmode then
          begin               {if unable to overlap
                               disk i/o and comm i/o.}
               send(XOFF);    {stop transmitter}
               delay(250);    {give it a chance}
          end;
          BlockWrite(Blkfile,dbuffer,bufcurr,bresult);
          if wxmode and pcjrmode then
          begin
               flush(blkfile); {complete all i/o}
               send(XON);      {restart transmitter}
          end;
          if bufcurr <> bresult then
          begin
               EotFlag := False;                {no longer a 'real' eot}
               trwindow(8,'Disk write error at end of receive');
          end;
     end;
     If Eotflag and Openflag then
     begin
          tblks := blkcnt;
          {$I-} close(blkfile); {$I+}
          If IOresult = 0 then
             good := true
          else writeln('WXMODEM - error closing receive file');
     end;
  99:
     if not Eotflag then
     begin
          if (errors >= Maxerrs) then
               trwindow(8,'Maximum errors exceeded')
          else
          if UserKey = CAN then
          begin
               trwindow(5,'^X entered');
               send(CAN); send(CAN); send(CAN);
          end;
          if (firstchar = CAN) then
               trwindow(5,'Cancel received');
          if openflag then
          begin
               {$I-} close(blkfile) {$I+};
               i := IOresult;                     {clear ioresult}
               {$I-} erase(blkfile); {$I+}
               i := IOresult;                     {clear ioresult}
          end;
     end;
     dbits        := db;
     parity       := p;
     stop_bits    := sb;
{    close(trfile);}
     update_uart;
     trwindow(99,'  ');
end;

procedure send_wcp;
Label
  tran,99;
Var
   UserKey                    : byte;
   c, i, j, sectnum, errors   : integer;
   sblks, ackblks, rblks : integer;        {total, sent, ack'd blocks}
   twindow, awindow           : integer;          {transmission window}
   bresult, nblks, prevchar   : integer;
   bflag, canflag, xpause     : boolean;
   extend                     : boolean;
   blkfile                    : file;
   statstr, statstr2          : bigstring;
   xblk, ackseq               : integer;
   trfile                     : text;

procedure checkack(tlimit : integer);

var
inchar  :   integer;

begin
   repeat                                      {until no more data & timelimit}
      inchar := cgetc(0);
      if inchar <> -1 then
      begin                                     {got a character}
         if wxmode then                         {wxmodem}
         begin
{           write(trfile,inchar:4);}
            case inchar of
               XOFF : begin
                         xpause := true;
                         txwindow(8,'Received - waiting');
                      end;
               XON  : begin
                         xpause := false;
                         txwindow(8,'No');
                      end;
               ACK, NAK, CAN : prevchar := inchar;       {save ACK/NAK/CAN}

               0..3 : begin                     {valid ACK/NAK sequence number}
                         case prevchar of
                            ACK : begin
                                     ackseq := inchar - (ackblks and twindow);
                                     if ackseq <= 0 then
                                        ackseq := ackseq + maxwindow;
                                     nblks := ackblks + ackseq;
                                     if nblks <= sblks then
                                     begin
                                        ackblks := nblks;
                                        str(ackblks:4,statstr);
                                        txwindow(6,statstr);
                                        if errors <> 0 then
                                        begin
                                           errors := 0;
                                           txwindow(10,'0');
                                        end;
                                     end;
{                                    writeln(trfile,' ACK ',inchar:2,ackblks:5);}
                                     prevchar := -1;
                                  end;                 {case ACK}
                            NAK : begin
                                     ackseq := inchar - (ackblks and twindow);
                                     if ackseq <= 0 then
                                        ackseq := ackseq + maxwindow;
                                     nblks := ackblks + ackseq;
                                     if nblks <= sblks then
                                     begin
                                        sblks := nblks - 1;
                                        if (sblks - ackblks) <= 2 then
                                           ackblks := sblks;
                                        str(nblks:4,statstr);
                                        txwindow(7,statstr);
                                        str(sblks:4,statstr);
                                        txwindow(5,statstr);
                                        errors := errors + 1;
                                        str(errors:3,statstr);
                                        txwindow(10,statstr);
                                     end
                                     else
                                     begin
                                       GotoXY(3,12);
                                       ClrEol;
                                       writeln('Invalid NAK seq ',nblks:4,ackseq:4,inchar:3);
                                     end;
{                                    writeln(trfile,' NAK ',inchar:2,ackblks:5,sblks:5);}
                                     prevchar := -1;
                                  end;                 {case NAK}
                            CAN : begin
                                     if inchar = CAN then
                                        canflag := true;
                                  end;
                         end;                          {of case prevchar}
                      end;                             {case 0..3}
               else                                    {of case inchar}
                  prevchar := -1;       {inchar not XON/XOFF/ACK/NAK/CAN/0/1/2/3}
            end;                                {of case inchar}
         end                                    {wxmodem mode}
         else
         begin                                  {regular xmodem}
            case inchar of
               ACK : begin
                        ackblks := ackblks + 1;
                        errors  := 0;
                     end;
               NAK : begin
                        sblks   := sblks - 1;
                        errors  := errors + 1;
                     end;
               CAN : begin
                        if prevchar = CAN then
                           canflag := true;
                        prevchar   := CAN;
                     end;
            else     prevchar := inchar;
            end;                                {end of case inchar}
         end;                                   {regular xmodem}
      end                                       {end of got a character}
      else                                      {no incoming data, inchar=-1}
      begin
         if tlimit > 0 then
         begin
            delay(1);
            tlimit := tlimit - 1;
         end;
      end;                                      {end no incoming data}
      if scan(Extend,UserKey) then
      begin
         if UserKey = CAN then
         begin
            canflag := true;
            tlimit  := 0;                       {force end of repeat}
            inchar  := -1;                      { "    "   "  "     }
            xpause  := false;
            purge;
         end;
      end;                                      {end of keypressed}
   until (tlimit <= 0) and (inchar = -1);       {repeat until nothing left}
end;                                            {of procedure checkack}

procedure ChkXoff;
var
  j : integer;
begin
   j := 0;
   repeat
      checkack(0);
      j := j + 1;
      delay(1);
   until ((xpause = false) or (j = 10000));
   if xpause then                      {but not forever}
      begin
         txwindow(8,'No - Timed Out');
         xpause := false;
      end;
end;

procedure dlesend(c:integer);
begin
   if wxmode then
   begin
      if buf_start <> buf_end then              {if there is any incoming data}
         checkack(0);
      if xpause then ChkXoff;                   {X-Off received .. better wait}
      case c of
         SYN, XON, XOFF, DLE :  begin
                                   send(DLE);
                                   send(c xor 64);
                                end;
                            else   send(c);
      end;
   end
   else send(c);                                {regular xmodem}
end;


begin
     status(2, 'SEND XMODEM');
     SaveCommStatus;
     Windowfl := false;
     openflag := false;
{    assign(trfile,'trace');}
{    rewrite(trfile);}
    fname := xmofile;
    Assign(Blkfile,fname);
    {I-} Reset(Blkfile); {I+}
    If IOresult <> 0 then
       goto 99;
    openflag := true;
    txwindow(1,fname);
    tblks := Trunc(LongFileSize(Blkfile));
    fmin := Trunc((tblks)*22.3333333/speed);
    str(fmin:3,statstr);
    fsec := Trunc((tblks*22.3333333/speed - fmin) * 60);
    str(fsec:2,statstr2);
    txwindow(3,statstr+':'+statstr2);
    str(tblks:4,statstr);
    txwindow(4,statstr);
    txwindow(12,'Press ^X to abort transfer');
    prevchar := -1;
    sblks   := 0;                               {sent blks}
    ackblks := 0;                               {ack'd blocks}
    rblks   := 0;                               {highest read block}
    errors  := 0;
    canflag := false;                           {not cancelled yet}
    xpause  := false;
    UserKey := 0;

                      {Xmodem transmit protocol initialization}

    i := 0;
    repeat
      c := cgetc(1);
      if c <> -1 then
      begin                                     {we got a character!}
           i := i + 1;                          {one of our 10 characters}
           case c of
             NAK   :  begin                     {Checksum Xmodem}
                           crcmode := false;
                           wxmode  := false;
                           twindow := 0;
                           txwindow(2,'Checksum Xmodem Send');
                           goto tran;
                      end;
             CHARC :  begin                     {CRC Xmodem}
                           crcmode := true;
                           wxmode  := false;
                           twindow := 0;
                           txwindow(2,'CRC Xmodem Send');
                           goto tran;
                      end;
             CHARW :  begin                     {WXmodem}
                           crcmode := true;
                           wxmode  := true;
                           twindow := Maxwindow - 1;
                           txwindow(2,'WXmodem Send');
                           str(Maxwindow:1,statstr);
                           txwindow(9,statstr);
                           goto tran;
                      end;

             CAN   :  begin                     {Cancel request received}
                           if canflag then goto 99
                           else canflag := true;
                      end;
           end;                                 {of case c}
       end;                                     {got a character}

       if scan(Extend, UserKey) then ;
    until (i > 10) or (UserKey = CAN);
    if UserKey = CAN then goto 99;
    UserKey := 0;
    txwindow(10,'Could not start: cancelled');
    purge;
    goto 99;

tran:                                           {let's send the file!}
    awindow := twindow;
    errors  := 0;
              {Xmodem packet level loop}

    while (ackblks < tblks) and (errors <= MAXERRS) do
    begin
       i := 0;
       while (sblks - ackblks) > awindow do     {is the ack window open?}
       begin                                    {no, so wait for ack/nak}
          i := i + 1;
          if i <= 1 then
          begin
             str((awindow+1):1,statstr);
             txwindow(9,concat(statstr,' Closed'));
          end;
          checkack(50);                         {50*2400 = 120 seconds +}
          if canflag then
             goto 99;
          if scan(Extend,UserKey) then
             if UserKey = CAN then
                goto 99;
          if i > 2400 then
          begin
             txwindow(11,'Timeout for ack');
             sblks := ackblks + 1;
             if sblks > tblks then
                goto 99;
          end;
          if (sblks - ackblks) <= awindow then
          begin
             str((awindow+1):1,statstr);
             txwindow(9,statstr);
          end;
       end;                                     {window closed}

       if sblks < tblks then                    {is there anything left?}
       begin
          awindow := twindow;                   {ack window is transmit window}
                           {disk read routine}
          sblks := sblks + 1;
          xblk  := sblks;
          while (xblk > rblks) or (xblk <= (rblks - bufnum)) do
          begin
             if xblk < (rblks - bufnum) then    {if we got nak'd back}
             begin
                seek(blkfile,(xblk-1));
             end;
             BlockRead(blkfile,dbuffer,bufnum,bresult);
             rblks := xblk + bufnum - 1;        {note rblks must go past eof}
          end;                                  {end of disk read routine}

          j := bufnum - rblks + xblk;           {index of next packet}

          crcval := 0;
          checksum := 0;
          if wxmode then
          begin
             if xpause then ChkXoff;
             send(SYN);
          end;
          dlesend(SOH);
          dlesend(xblk and 255);                {block sequence}
          dlesend((xblk and 255) xor 255);      {complement sequence}
          for i := 1 to 128 do
          begin
             c := dbuffer[j,i];
             if crcmode then updcrc(c)
             else checksum := (checksum + c) and 255;
             dlesend(c);
          end;
          if crcmode then
          begin
             dlesend(hi(crcval));
             dlesend(lo(crcval));
          end
          else
             send(checksum);
          if canflag then
             goto 99;
          str(xblk:4,statstr);
          txwindow(5,statstr);
{         writeln(trfile,'SENT ',sblks:5,xblk:5);}
       end                                      {something to send}
       else
       begin                                    {nothing else to send}
          if wxmode then
          begin
             str(awindow:1,statstr);
             txwindow(9,concat(statstr,' -- Closing'));
             awindow := sblks - ackblks - 1;    {wait for final acks}
          end;
       end;
    end;                                        {xmodem send routine}

    repeat                                      {end of transmission}
      send(EOT);
      UserKey := 0;
      repeat
        c := cgetc(15);
        if scan(Extend,UserKey) then ;
      until (c <> -1) or (UserKey = CAN);
      if UserKey = CAN then goto 99;
      if c = NAK then
      begin
         errors := errors + 1;
         delay(250);
      end;
    until (c = ACK) or (errors = MAXERRS);
    if errors = MAXERRS then
       txwindow(11,'ACK not received at EOT');
    good := true;       {Yea - we sent it}
    99:
{   close(trfile);}
    if openflag then
    begin
       {$I-} close(blkfile) {$I+} ;
       i := IOresult;                           {clear ioresult}
    end;
    if ((UserKey = CAN) or canflag) and (length(fname) > 0) then
    begin
      repeat
        send(CAN);
        send(CAN);
        purge
      until cgetc(1) = -1
    end;
    txwindow(99,'  ');
    dbits        := db;
    parity       := p;
    stop_bits    := sb;
    update_uart
end;