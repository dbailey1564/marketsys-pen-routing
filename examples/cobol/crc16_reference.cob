       >>source format free
identification division.
program-id. ms-crc-reference.
data division.
working-storage section.
01 ws-data             pic x(256).
01 ws-count            pic 9(9) comp-5.
01 ws-crc              pic 9(9) comp-5.
01 ws-expected         pic 9(9) comp-5.
01 ws-byte-index       pic 9(9) comp-5.
01 ws-byte-value       pic 9(9) comp-5.
01 ws-bit-index        pic 9(9) comp-5.
01 ws-top-bit          pic 9 comp-5.
01 ws-wide             pic 9(9) comp-5.
01 ws-quotient         pic 9(9) comp-5.
01 ws-xa               pic 9(9) comp-5.
01 ws-xb               pic 9(9) comp-5.
01 ws-xor-result       pic 9(9) comp-5.
01 ws-xor-index        pic 9(9) comp-5.
01 ws-ra               pic 9 comp-5.
01 ws-rb               pic 9 comp-5.
01 ws-weight           pic 9(9) comp-5.
01 ws-low              pic 9(9) comp-5.
01 ws-high             pic 9(9) comp-5.
01 ws-char-position    pic 9(9) comp-5.
01 ws-frame            pic x(16).
01 ws-hex              pic x(16) value "0123456789ABCDEF".
01 ws-frame-hex        pic x(32).
01 ws-out-position     pic 9(9) comp-5.
01 ws-vector-name      pic x(32).
procedure division.
main.
    *> These assumptions must hold before ORD/CHAR are used for raw bytes.
    if function ord(x"00") not = 1
        or function ord(x"80") not = 129
        or function ord(x"FF") not = 256
        or function char(1) not = x"00"
        or function char(129) not = x"80"
        or function char(256) not = x"FF"
        display "FAIL: adapt byte conversion to compiler collating sequence"
        move 1 to return-code
        goback
    end-if

    move "Empty input" to ws-vector-name
    move 0 to ws-count
    move 65535 to ws-expected
    perform calculate-crc
    perform assert-crc

    *> Hex literal deliberately supplies ASCII bytes even on another host.
    move "ASCII 123456789" to ws-vector-name
    move x"313233343536373839" to ws-data
    move 9 to ws-count
    move 10673 to ws-expected
    perform calculate-crc
    perform assert-crc

    move "HEAD pen20" to ws-vector-name
    move x"AA550210785634120300000014000000" to ws-frame
    move ws-frame(3:12) to ws-data
    move 12 to ws-count
    move 2869 to ws-expected
    perform calculate-crc
    perform assert-crc

    *> Split numeric CRC explicitly; never move native COMP-5 storage to wire.
    divide ws-crc by 256 giving ws-high remainder ws-low
    compute ws-char-position = ws-low + 1
    move function char(ws-char-position) to ws-frame(15:1)
    compute ws-char-position = ws-high + 1
    move function char(ws-char-position) to ws-frame(16:1)
    if ws-frame not = x"AA55021078563412030000001400350B"
        display "FAIL: frame serialization"
        move 1 to return-code
        goback
    end-if
    perform format-frame
    display "FRAME=" ws-frame-hex

    move "HEAD pen20B" to ws-vector-name
    move x"021078563412030000001442" to ws-data
    move 12 to ws-count
    move 25523 to ws-expected
    perform calculate-crc
    perform assert-crc

    move "All bytes 00..FF" to ws-vector-name
    perform varying ws-byte-index from 1 by 1
        until ws-byte-index > 256
        move function char(ws-byte-index) to ws-data(ws-byte-index:1)
    end-perform
    move 256 to ws-count
    move 16317 to ws-expected
    perform calculate-crc
    perform assert-crc
    display "PASS: CRC vectors, byte conversion, frame and CRC byte order"
    move 0 to return-code
    goback.

calculate-crc.
    *> Input: ws-data raw bytes; ws-count exact count (0..256).
    *> Output: ws-crc numeric 0..65535. No extra zero bytes or final XOR.
    if ws-count > 256
        display "FAIL: CRC input length exceeds buffer"
        move 1 to return-code
        goback
    end-if
    move 65535 to ws-crc
    perform varying ws-byte-index from 1 by 1
        until ws-byte-index > ws-count
        compute ws-byte-value =
            function ord(ws-data(ws-byte-index:1)) - 1
        move ws-crc to ws-xa
        compute ws-xb = ws-byte-value * 256
        perform xor-16
        move ws-xor-result to ws-crc
        perform varying ws-bit-index from 1 by 1 until ws-bit-index > 8
            move 0 to ws-top-bit
            if ws-crc >= 32768 move 1 to ws-top-bit end-if
            compute ws-wide = ws-crc * 2
            divide ws-wide by 65536
                giving ws-quotient remainder ws-crc
            if ws-top-bit = 1
                move ws-crc to ws-xa
                move 4129 to ws-xb
                perform xor-16
                move ws-xor-result to ws-crc
            end-if
        end-perform
    end-perform.

xor-16.
    *> Arithmetic XOR: no vendor-specific bitwise intrinsic required.
    *> Inputs ws-xa/ws-xb are consumed. Result is ws-xor-result.
    move 0 to ws-xor-result
    move 1 to ws-weight
    perform varying ws-xor-index from 1 by 1 until ws-xor-index > 16
        divide ws-xa by 2 giving ws-xa remainder ws-ra
        divide ws-xb by 2 giving ws-xb remainder ws-rb
        if ws-ra not = ws-rb add ws-weight to ws-xor-result end-if
        multiply 2 by ws-weight
    end-perform.

assert-crc.
    if ws-crc not = ws-expected
        display "FAIL " function trim(ws-vector-name)
            " expected=" ws-expected " got=" ws-crc
        move 1 to return-code
        goback
    end-if
    display "PASS " function trim(ws-vector-name) " CRC-decimal=" ws-crc.

format-frame.
    *> Printable hex is for diagnostics only, not the serial transmit data.
    perform varying ws-byte-index from 1 by 1 until ws-byte-index > 16
        compute ws-byte-value = function ord(ws-frame(ws-byte-index:1)) - 1
        divide ws-byte-value by 16 giving ws-high remainder ws-low
        compute ws-out-position = ws-byte-index * 2 - 1
        compute ws-char-position = ws-high + 1
        move ws-hex(ws-char-position:1) to ws-frame-hex(ws-out-position:1)
        add 1 to ws-out-position
        compute ws-char-position = ws-low + 1
        move ws-hex(ws-char-position:1) to ws-frame-hex(ws-out-position:1)
    end-perform.
end program ms-crc-reference.
