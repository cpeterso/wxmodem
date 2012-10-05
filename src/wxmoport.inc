{$U-,C-,I-,K-}
type
    tCport = (com1, com2);
    tSaveInt = record                  {Save interrupt vector address}
       IP :  integer;
       CS :  integer;
    end;
    tRegs    = record                  {Dos registers}
       AX, BX, CX, DX, BP, SI, DI, DS, ES, Flags : integer;
    end;
Const
     RECV_BUF_SIZE = 2048;             {this may be changed to
                                        whatever size you need}
{ *** Port addresses *** }
     cRBR: array[com1..com2] of integer = ($3F8,$2F8);
                                       {Receive Buffer Register}
     cTHR: array[com1..com2] of integer = ($3F8,$2F8);
                                       {Transmitter Holding Register: the
                                        serial port address we use to send
                                        data}
     cIER: array[com1..com2] of integer = ($3F9,$2F9);
                                       {Interrupt Enable Register for the
                                        serial port}
     cLCR: array[com1..com2] of integer = ($3FB,$2FB);
                                       {Line Control Register for the serial
                                        port. Determines data bits, stop bits
                                        and parity, contributes to setting
                                        baud-rate}
     cMCR: array[com1..com2] of integer = ($3FC,$2FC);
                                       {Modem Control Register}
     cLSR: array[com1..com2] of integer = ($3FD,$2FD);
                                       {Line Status Register}
     cMSR: array[com1..com2] of integer = ($3FE,$2FE);
                                       {Modem Status Register}
     IMR = $021;                       {Interrupt Mask Register port address
                                        of Intel 8259A Programmable Interrupt
                                        controller}
{ *** Masks *** }
     ENABLE_OUT2 = 8;                  {Setting bit 3 of MCR enables OUT2}
     ENABLE_DAV = 1;                   {Setting bit 0 of IER enables Data
                                        AVailable interrupt from serial port}
     ENABLE_IRQ: array[com1..com2] of integer = ($00EF,$00F7);
                                       {Clearing bit of IMR enables serial
                                        interrupts to reach the CPU}
     DISABLE_OUT2 = 1;                 {Clearing MCR disables OUT2}
     DISABLE_DAV = 0;                  {Clearing IER disables Data
                                       AVailable interrupt from serial port}
     DISABLE_IRQ: array[com1..com2] of integer = ($0010,$0008);
                                       {Setting bit of IMR stops serial
                                        interrupts from reaching the CPU}
     cINVLIST: array[com1..com2] of integer = ($000C,$000B);
                                       {Interrupt vector number}
     SET_BAUD = $80;                   {Setting bit 7 of LCR allows us to set
                                        the baud rate of the serial port}
     SET_PARMS = $7F;                  {Clearing bit 7 of LCR allows us to set
                                        non-baud-rate parameters on the
                                        serial port}
Type
    parity_set        = (none,even);    {readability and expansion}
Var
   SaveInt                           : tSaveInt;
   Regs                              : tRegs;
   RBR, THR, IER, LCR, MCR, LSR, MSR : integer;
   INVLIST                           : integer;
   SavIER, SavLCR, SavMCR, SavIMR    : integer;
   buf_start, buf_end    : integer;    {NOTE: these will change by them-
                                        selves in the background}
   recv_buffer           : array [1..RECV_BUF_SIZE] of byte;
                                       {also self-changing}
   speed                 : integer;    {I don't know the top speed these
                                        routines will handle}
   dbits                 : 7..8;       {only ones most people use}
   stop_bits             : 1..2;       {does anyone use 2?}
   parity                : parity_set;  {even and none are the common ones}
   Cport                 : tCport;     {set at initialization}

function cgetc(TimeLimit : integer) : integer;
{if a byte is recieved at COM1/COM2: in less than TimeLimit seconds,
 returns byte as an integer, else returns -1}
const
     TIMED_OUT = -1;
begin
     TimeLimit := TimeLimit shl 10;     {convert TimeLimit to millisecs}
     while (buf_start = buf_end) and (TimeLimit > 0) do
     begin
          delay(1);
          TimeLimit := pred(TimeLimit)
     end;
     if (TimeLimit >= 0) and (buf_start <> buf_end) then
     begin
          inline ($FA);            {suspend interrupts}
          cgetc := recv_buffer[buf_start];
          buf_start := succ(buf_start);
          if buf_start > RECV_BUF_SIZE then
          buf_start := 1;
          inline ($FB);            {resume interrupts}
     end
     else
         cgetc := TIMED_OUT;
end;

procedure send(c : byte);
var
   a : byte;
begin
{
  repeat
       a := port[LSR]
  until odd(a shr 5);
  port[THR] := c;
 }
  inline(
     $8B/$16/LSR/   {in: mov dx,[LSR]}
     $EC/               {in  al,dx}
     $24/$20/           {and al,20}
     $74/$F7/           {jz  in}
     $8B/$16/THR/       {mov dx,[THR]}
     $36/               {ss:}
     $8B/$86/c/         {mov ax,[bp+c]}
     $EE                {out dx,al}
        );
end;


{Communications routines for TURBO Pascal written by Alan Bishop,
 modified slightly by Scott Murphy.  Modified by Peter Boswell to
 add COM2, saving the original interrupt vector, more inline code.
 Handles standart COM1/COM2: ports with interrupt handling.  Includes
 support for only one port, and with no overflow, parity, or other
 such checking.  However, even some of the best communication programs
 don't do this anyway, and I never use it.  If you make modifications,
 please send me a copy if you have a simple way of doing it (CIS EMAIL,
 Usenet, MCI Mail, etc)  Hope these are useful.

Alan Bishop - CIS      - 72405,647
              Usenet   - bishop@ecsvax
              MCI Mail - ABISHOP
}
procedure update_uart;
{uses dbits, stop_bits, and parity}
var
   newparm, oldLCR : byte;
begin
 newparm := dbits-5;
 if stop_bits = 2 then newparm := newparm + 4;
 if parity = even then newparm := newparm + 24;
 oldLCR := port[LCR];
 port[LCR] := oldLCR and SET_PARMS;
 port[LCR] := newparm;
end;

procedure term_ready(state : boolean);
{if state = TRUE then set RTS true else set false}
var
   OldMCR : byte;
begin
     OldMCR := port[MCR];
     if state then
        port[MCR] := OldMCR or 1
     else
         port[MCR] := OldMCR and $FE
end;

function carrier : boolean;
{true if carrier, false if not}
begin
 carrier := odd(port[MSR] shr 7);
end;

procedure set_up_recv_buffer;
begin
 buf_start := 1;
 buf_end   := 1;
end;

procedure new_baud(rate : integer);
{has no problems with non-standard bauds}
var
   OldLCR : byte;
begin
 if rate <= 9600 then
 begin
  speed := rate;
  rate := trunc(115200.0/rate);
  OldLCR := port[LCR] or SET_BAUD;
  port[LCR] := OldLCR;
  port[THR] := lo(rate);
  port[IER] := hi(rate);
  port[LCR] := OldLCR and SET_PARMS;
 end;
end;

procedure init_port;
{installs interrupt sevice routine for serial port}
var a,b : integer;
    buf_len : integer;
begin
 RBR := cRBR[Cport];
 THR := cTHR[Cport];
 IER := cIER[Cport];
 LCR := cLCR[Cport];
 MCR := cMCR[Cport];
 LSR := cLSR[Cport];
 MSR := cMSR[Cport];
 INVLIST := cINVLIST[Cport];
 update_uart;
 new_baud(speed);
 buf_len := RECV_BUF_SIZE;

 {save the original vector}

 with Regs do begin
   AX := $3500 + INVLIST;
   MsDos(Regs);
   SaveInt.CS := ES;
   SaveInt.IP := BX;
 end;

 {this is the background routine}

 inline (
              $FA/                     {cli}
              $1E/                     {push ds}
              $0E/                     {push cs}
              $A1/INVLIST/             {mov  ax,[INVLIST]   interrupt vector}
              $1F/                     {pop  ds                  ;ds := cs}
              $BA/*+22/                {mov  dx, offset ISR}
              $B4/$25/                 {mov  ah,25H}
              $CD/$21/                 {int  21H}
              $8B/$BE/BUF_LEN/         {mov  di, buf_len}
              $89/$3E/*+88/            {mov  lcl_buf_len,di}
              $1F/                     {pop  ds}
              $2E/$8C/$1E/*+84/        {mov  lcl_ds, ds}
              $EB/$52/                 {jmp  exit}
{ISR:}        $1E/                     {push ds}
              $50/                     {push ax}
              $53/                     {push bx}
              $52/                     {push dx}
              $56/                     {push si}
              $FB/                     {sti}
              $2E/$8E/$1E/*+71/        {mov  ds,[lcl_ds]}
              $8B/$16/RBR/             {mov  dx,[RBR] 3F8H/2F8H ;address RBR}
              $EC/                     {in   al, dx             ;read rbr}
              $BE/RECV_BUFFER/         {mov  si, recv_buffer    ;address start of recv_buffer}
              $8B/$1E/BUF_END/         {mov  bx, [buf_end]      ;index of current char in recv_buffer}
              $88/$40/$FF/             {mov  [bx+si-1],al       ;copy char to recv_buffer}
              $43/                     {inc  bx                 ;update buf_end}
              $E8/$22/$00/             {call adj_idx}
              $89/$1E/BUF_END/         {mov  [buf_end],bx}
              $3B/$1E/BUF_START/       {cmp  bx, [buf_start]}
              $75/$0C/                 {jnz  ISR_DONE}
              $8B/$1E/BUF_START/       {mov  bx,buf_start}
              $43/                     {inc  bx}
              $E8/$10/$00/             {call adj_idx}
              $89/$1E/BUF_START/       {mov  [buf_start],bx}
              $BA/$20/$00/             {mov  dx,20H            ;EOI command for 8259A PIC}
              $B0/$20/                 {mov  al,20H            ;EOI port for 8259A PIC}
              $EE/                     {out  dx,al             ;End Of Interrupt}
              $5E/                     {pop  si}
              $5A/                     {pop  dx}
              $5B/                     {pop  bx}
              $58/                     {pop  ax}
              $1F/                     {pop  ds}
              $CF/                     {iret}
{adj_idx:}    $2E/$8B/$16/*+11/        {mov  dx,[lcl_buf_len]}
              $42/                     {inc  dx}
              $39/$DA/                 {cmp  dx,bx}
              $75/$03/                 {jnz  no_change}
              $BB/$01/$00/             {mov  bx,1}
{no_change:}  $C3/                     {ret}
{lcl_buf_len;}$00/$00/                 {dw  0}
              $00/$01/                 {dw  1}
{exit:}       $90/                     {nop}
              $FB                      {cli}
 );
 SavLCR    := port[LCR];
 SavIER    := port[IER];
 port[IER] := ENABLE_DAV;              {interrupt enable}
 a         := port[MCR];
 SavMCR    := a;
 port[MCR] := a or ENABLE_OUT2;        {preserve RTS and enable OUT2}
 a         := port[IMR];
 SavIMR    := a;
 a         := a and ENABLE_IRQ[Cport];
 port[IMR] := a;
end;


procedure remove_port;
{Restores status to reflect the status upon entry}
var
   a : byte;
begin
     inline($FA);                     {disable interrupts}
     port[IMR] := SavIMR;
     port[IER] := SavIER;
     port[MCR] := SavMCR;
     port[LCR] := SavLCR;

     {Restore the interrupt vector}
     with Regs do begin
        AX := $2500 + INVLIST;        {function 25H & INVLIST port}
        DS := SaveInt.CS;
        DX := SaveInt.IP;
        MsDos(Regs);
     end;
     inline($FB);
end;


procedure break;
{send a break}
var a,b : byte;
begin
 a := port[LCR];
 b := (a and $7F) or $40;
 port[LCR] := b;
 delay(750);
 port[LCR] := a;
end;