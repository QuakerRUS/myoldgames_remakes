DEFINT A-Z
SCREEN 13: CLS
DIM grass(33), hor1(33), hor2(33), hor3(33), hor4(33), ver1(33), ver2(33), ver3(33), ver4(33), head1u(33), head1r(33), head1d(33), head1l(33), head2u(33), head2r(33), head2d(33), head2l(33), head3u(33), head3r(33), head3d(33), head3l(33), head4u(33) _
, head4r(33), head4d(33), head4l(33), head5u(33), head5r(33), head5d(33), head5l(33), head6u(33), head6r(33), head6d(33), head6l(33), head7u(33), head7r(33), head7d(33), head7l(33), tail1u(33), tail1r(33), tail1d(33), tail1l(33), tail2u(33), tail2r( _
33)
DIM tail2d(33), tail2l(33), tail3u(33), tail3r(33), tail3d(33), tail3l(33), tail4u(33), tail4r(33), tail4d(33), tail4l(33), tail5u(33), tail5r(33), tail5d(33), tail5l(33), ru1(33), ru2(33), ru3(33), ru4(33), rd1(33), rd2(33), rd3(33), rd4(33), ld1( _
33), ld2(33), ld3(33), ld4(33), lu1(33), lu2(33), lu3(33), lu4(33), hul1(33), hul2(33), hul3(33), hul4(33), hul5(33), hur1(33), hur2(33), hur3(33), hur4(33), hur5(33), hdr1(33), hdr2(33), hdr3(33), hdr4(33), hdr5(33), hdl1(33), hdl2(33), hdl3(33),  _
hdl4(33), hdl5(33), hlu1(33), hlu2(33), hlu3(33), hlu4(33), hlu5(33), hru1(33), hru2(33), hru3(33), hru4(33), hru5(33), hrd1(33), hrd2(33), hrd3(33), hrd4(33), hrd5(33), hld1(33), hld2(33), hld3(33), hld4(33), hld5(33), tul1(33), tul2(33), tur1(33) _
, tur2(33), tdr1(33), tdr2(33), tdl1(33), tdl2(33), tlu1(33), tlu2(33), tru1(33), tru2(33), trd1(33), trd2(33), tld1(33), tld2(33), rabbit1(33), rabbit2(33), head5ur(33), head5rr(33), head5dr(33), head5lr(33), head6ur(33), head6rr(33), head6dr(33),  _
head6lr(33), head7ur(33), head7rr(33), head7dr(33), head7lr(33), hul4r(33), hul5r(33), hur4r(33), hur5r(33), hdr4r(33), hdr5r(33), hdl4r(33), hdl5r(33), hlu4r(33), hlu5r(33), hru4r(33), hru5r(33), hrd4r(33), hrd5r(33), hld4r(33), hld5r(33), ml(38,  _
23)
GET (0, 0)-(7, 7), grass
GET (0, 0)-(7, 7), hor1
GET (0, 0)-(7, 7), hor2
GET (0, 0)-(7, 7), hor3
GET (0, 0)-(7, 7), hor4
GET (0, 0)-(7, 7), ver1
GET (0, 0)-(7, 7), ver2
GET (0, 0)-(7, 7), ver3
GET (0, 0)-(7, 7), ver4
GET (0, 0)-(7, 7), head1u
GET (0, 0)-(7, 7), head1r
GET (0, 0)-(7, 7), head1d
GET (0, 0)-(7, 7), head1l
GET (0, 0)-(7, 7), head2u
GET (0, 0)-(7, 7), head2r
GET (0, 0)-(7, 7), head2d
GET (0, 0)-(7, 7), head2l
GET (0, 0)-(7, 7), head3u
GET (0, 0)-(7, 7), head3r
GET (0, 0)-(7, 7), head3d
GET (0, 0)-(7, 7), head3l
GET (0, 0)-(7, 7), head4u
GET (0, 0)-(7, 7), head4r
GET (0, 0)-(7, 7), head4d
GET (0, 0)-(7, 7), head4l
GET (0, 0)-(7, 7), head5u
GET (0, 0)-(7, 7), head5r
GET (0, 0)-(7, 7), head5d
GET (0, 0)-(7, 7), head5l
GET (0, 0)-(7, 7), head6u
GET (0, 0)-(7, 7), head6r
GET (0, 0)-(7, 7), head6d
GET (0, 0)-(7, 7), head6l
GET (0, 0)-(7, 7), head7u
GET (0, 0)-(7, 7), head7r
GET (0, 0)-(7, 7), head7d
GET (0, 0)-(7, 7), head7l
GET (0, 0)-(7, 7), tail1u
GET (0, 0)-(7, 7), tail1r
GET (0, 0)-(7, 7), tail1d
GET (0, 0)-(7, 7), tail1l
GET (0, 0)-(7, 7), tail2u
GET (0, 0)-(7, 7), tail2r
GET (0, 0)-(7, 7), tail2d
GET (0, 0)-(7, 7), tail2l
GET (0, 0)-(7, 7), tail3u
GET (0, 0)-(7, 7), tail3r
GET (0, 0)-(7, 7), tail3d
GET (0, 0)-(7, 7), tail3l
GET (0, 0)-(7, 7), tail4u
GET (0, 0)-(7, 7), tail4r
GET (0, 0)-(7, 7), tail4d
GET (0, 0)-(7, 7), tail4l
GET (0, 0)-(7, 7), tail5u
GET (0, 0)-(7, 7), tail5r
GET (0, 0)-(7, 7), tail5d
GET (0, 0)-(7, 7), tail5l
GET (0, 0)-(7, 7), ru1
GET (0, 0)-(7, 7), ru2
GET (0, 0)-(7, 7), ru3
GET (0, 0)-(7, 7), ru4
GET (0, 0)-(7, 7), rd1
GET (0, 0)-(7, 7), rd2
GET (0, 0)-(7, 7), rd3
GET (0, 0)-(7, 7), rd4
GET (0, 0)-(7, 7), ld1
GET (0, 0)-(7, 7), ld2
GET (0, 0)-(7, 7), ld3
GET (0, 0)-(7, 7), ld4
GET (0, 0)-(7, 7), lu1
GET (0, 0)-(7, 7), lu2
GET (0, 0)-(7, 7), lu3
GET (0, 0)-(7, 7), lu4
GET (0, 0)-(7, 7), hul1
GET (0, 0)-(7, 7), hul2
GET (0, 0)-(7, 7), hul3
GET (0, 0)-(7, 7), hul4
GET (0, 0)-(7, 7), hul5
GET (0, 0)-(7, 7), hur1
GET (0, 0)-(7, 7), hur2
GET (0, 0)-(7, 7), hur3
GET (0, 0)-(7, 7), hur4
GET (0, 0)-(7, 7), hur5
GET (0, 0)-(7, 7), hdr1
GET (0, 0)-(7, 7), hdr2
GET (0, 0)-(7, 7), hdr3
GET (0, 0)-(7, 7), hdr4
GET (0, 0)-(7, 7), hdr5
GET (0, 0)-(7, 7), hdl1
GET (0, 0)-(7, 7), hdl2
GET (0, 0)-(7, 7), hdl3
GET (0, 0)-(7, 7), hdl4
GET (0, 0)-(7, 7), hdl5
GET (0, 0)-(7, 7), hlu1
GET (0, 0)-(7, 7), hlu2
GET (0, 0)-(7, 7), hlu3
GET (0, 0)-(7, 7), hlu4
GET (0, 0)-(7, 7), hlu5
GET (0, 0)-(7, 7), hru1
GET (0, 0)-(7, 7), hru2
GET (0, 0)-(7, 7), hru3
GET (0, 0)-(7, 7), hru4
GET (0, 0)-(7, 7), hru5
GET (0, 0)-(7, 7), hrd1
GET (0, 0)-(7, 7), hrd2
GET (0, 0)-(7, 7), hrd3
GET (0, 0)-(7, 7), hrd4
GET (0, 0)-(7, 7), hrd5
GET (0, 0)-(7, 7), hld1
GET (0, 0)-(7, 7), hld2
GET (0, 0)-(7, 7), hld3
GET (0, 0)-(7, 7), hld4
GET (0, 0)-(7, 7), hld5
GET (0, 0)-(7, 7), tul1
GET (0, 0)-(7, 7), tul2
GET (0, 0)-(7, 7), tur1
GET (0, 0)-(7, 7), tur2
GET (0, 0)-(7, 7), tdr1
GET (0, 0)-(7, 7), tdr2
GET (0, 0)-(7, 7), tdl1
GET (0, 0)-(7, 7), tdl2
GET (0, 0)-(7, 7), tlu1
GET (0, 0)-(7, 7), tlu2
GET (0, 0)-(7, 7), tru1
GET (0, 0)-(7, 7), tru2
GET (0, 0)-(7, 7), trd1
GET (0, 0)-(7, 7), trd2
GET (0, 0)-(7, 7), tld1
GET (0, 0)-(7, 7), tld2
GET (0, 0)-(7, 7), rabbit1
GET (0, 0)-(7, 7), rabbit2
GET (0, 0)-(7, 7), head5ur
GET (0, 0)-(7, 7), head5rr
GET (0, 0)-(7, 7), head5dr
GET (0, 0)-(7, 7), head5lr
GET (0, 0)-(7, 7), head6ur
GET (0, 0)-(7, 7), head6rr
GET (0, 0)-(7, 7), head6dr
GET (0, 0)-(7, 7), head6lr
GET (0, 0)-(7, 7), head7ur
GET (0, 0)-(7, 7), head7rr
GET (0, 0)-(7, 7), head7dr
GET (0, 0)-(7, 7), head7lr
GET (0, 0)-(7, 7), hul4r
GET (0, 0)-(7, 7), hul5r
GET (0, 0)-(7, 7), hur4r
GET (0, 0)-(7, 7), hur5r
GET (0, 0)-(7, 7), hdr4r
GET (0, 0)-(7, 7), hdr5r
GET (0, 0)-(7, 7), hdl4r
GET (0, 0)-(7, 7), hdl5r
GET (0, 0)-(7, 7), hlu4r
GET (0, 0)-(7, 7), hlu5r
GET (0, 0)-(7, 7), hru4r
GET (0, 0)-(7, 7), hru5r
GET (0, 0)-(7, 7), hrd4r
GET (0, 0)-(7, 7), hrd5r
GET (0, 0)-(7, 7), hld4r
GET (0, 0)-(7, 7), hld5r
GOSUB 1000
SCREEN 12: CLS
PALETTE 12, 0
PALETTE 0, 42
DRAW "c9bm10,10m60,170r30m140,10l20m75,150m30,10l20bm160,40u30r120d30bm-10,-10l40d110bm+10,+10d20l40u20bm+10,-10u110l40"
CIRCLE (170, 40), 10, 9, 1.57, 3.14
CIRCLE (270, 40), 10, 9, 0, 1.57
CIRCLE (200, 140), 10, 9, 4.71, 0
CIRCLE (240, 140), 10, 9, 3.14, 4.71
PAINT (75, 160), 14, 9
PAINT (220, 20), 14, 9
DRAW "c15bm390,128u20nr20u20r30br10r15nd40r15br10m+10,+40m+10,-40m+10,+40m+10,-40bm+10,+40m+5,-15nr20m535,88m+15,+40br10u40r10bd20nl10f20br10nr30u20nr30u20r30"
CIRCLE (325, 98), 15, 15, 0, 4.71, .66
CIRCLE (325, 118), 15, 15, 3.14, 1.57, .66
CIRCLE (365, 108), 20, 15, , , 1.33
CIRCLE (570, 98), 20, 15, 4.71, 1.57, .5
COLOR 10: LOCATE 10, 49: PRINT "E N T E R T A I M E N T"
COLOR 11: LOCATE 15, 33: PRINT "p r e s e n t s"
LINE (120, 300)-(510, 446), 14, B
LINE (123, 303)-(507, 443), 14, B
LINE (126, 306)-(504, 440), 14, B
COLOR 7: LOCATE 21, 18: PRINT "Program by Fedoseyenko Victor & Kulakov Taras"
LOCATE 24, 27: PRINT "Russia, Klintsy, 2000 a.d."
LOCATE 26, 30: PRINT "tel: (08336) 2-39-91"
LOCATE 27, 35: PRINT "(08336) 2-00-21"
LINE (236, 270)-(396, 290), 12, BF
RANDOMIZE VAL(LEFT$(TIME$, 2)) * 1000 + VAL(MID$(TIME$, 4, 2)) * 100 + VAL(RIGHT$(TIME$, 2))

OPEN "graph.dat" FOR INPUT AS #1
CHDIR "GRAPHICS"
num = 1
WHILE EOF(1) <> -1
INPUT #1, a$
OPEN a$ + ".dat" FOR RANDOM AS #2 LEN = 2
FIELD #2, 2 AS B$
FOR nowpos = 1 TO 33
GET #2, nowpos
IF a$ = "grass" THEN grass(nowpos) = CVI(B$)
IF a$ = "hor1" THEN hor1(nowpos) = CVI(B$)
IF a$ = "hor2" THEN hor2(nowpos) = CVI(B$)
IF a$ = "hor3" THEN hor3(nowpos) = CVI(B$)
IF a$ = "hor4" THEN hor4(nowpos) = CVI(B$)
IF a$ = "ver1" THEN ver1(nowpos) = CVI(B$)
IF a$ = "ver2" THEN ver2(nowpos) = CVI(B$)
IF a$ = "ver3" THEN ver3(nowpos) = CVI(B$)
IF a$ = "ver4" THEN ver4(nowpos) = CVI(B$)
IF a$ = "head1u" THEN head1u(nowpos) = CVI(B$)
IF a$ = "head1r" THEN head1r(nowpos) = CVI(B$)
IF a$ = "head1d" THEN head1d(nowpos) = CVI(B$)
IF a$ = "head1l" THEN head1l(nowpos) = CVI(B$)
IF a$ = "head2u" THEN head2u(nowpos) = CVI(B$)
IF a$ = "head2r" THEN head2r(nowpos) = CVI(B$)
IF a$ = "head2d" THEN head2d(nowpos) = CVI(B$)
IF a$ = "head2l" THEN head2l(nowpos) = CVI(B$)
IF a$ = "head3u" THEN head3u(nowpos) = CVI(B$)
IF a$ = "head3r" THEN head3r(nowpos) = CVI(B$)
IF a$ = "head3d" THEN head3d(nowpos) = CVI(B$)
IF a$ = "head3l" THEN head3l(nowpos) = CVI(B$)
IF a$ = "head4u" THEN head4u(nowpos) = CVI(B$)
IF a$ = "head4r" THEN head4r(nowpos) = CVI(B$)
IF a$ = "head4d" THEN head4d(nowpos) = CVI(B$)
IF a$ = "head4l" THEN head4l(nowpos) = CVI(B$)
IF a$ = "head5u" THEN head5u(nowpos) = CVI(B$)
IF a$ = "head5r" THEN head5r(nowpos) = CVI(B$)
IF a$ = "head5d" THEN head5d(nowpos) = CVI(B$)
IF a$ = "head5l" THEN head5l(nowpos) = CVI(B$)
IF a$ = "head6u" THEN head6u(nowpos) = CVI(B$)
IF a$ = "head6r" THEN head6r(nowpos) = CVI(B$)
IF a$ = "head6d" THEN head6d(nowpos) = CVI(B$)
IF a$ = "head6l" THEN head6l(nowpos) = CVI(B$)
IF a$ = "head7u" THEN head7u(nowpos) = CVI(B$)
IF a$ = "head7r" THEN head7r(nowpos) = CVI(B$)
IF a$ = "head7d" THEN head7d(nowpos) = CVI(B$)
IF a$ = "head7l" THEN head7l(nowpos) = CVI(B$)
IF a$ = "tail1u" THEN tail1u(nowpos) = CVI(B$)
IF a$ = "tail1r" THEN tail1r(nowpos) = CVI(B$)
IF a$ = "tail1d" THEN tail1d(nowpos) = CVI(B$)
IF a$ = "tail1l" THEN tail1l(nowpos) = CVI(B$)
IF a$ = "tail2u" THEN tail2u(nowpos) = CVI(B$)
IF a$ = "tail2r" THEN tail2r(nowpos) = CVI(B$)
IF a$ = "tail2d" THEN tail2d(nowpos) = CVI(B$)
IF a$ = "tail2l" THEN tail2l(nowpos) = CVI(B$)
IF a$ = "tail3u" THEN tail3u(nowpos) = CVI(B$)
IF a$ = "tail3r" THEN tail3r(nowpos) = CVI(B$)
IF a$ = "tail3d" THEN tail3d(nowpos) = CVI(B$)
IF a$ = "tail3l" THEN tail3l(nowpos) = CVI(B$)
IF a$ = "tail4u" THEN tail4u(nowpos) = CVI(B$)
IF a$ = "tail4r" THEN tail4r(nowpos) = CVI(B$)
IF a$ = "tail4d" THEN tail4d(nowpos) = CVI(B$)
IF a$ = "tail4l" THEN tail4l(nowpos) = CVI(B$)
IF a$ = "tail5u" THEN tail5u(nowpos) = CVI(B$)
IF a$ = "tail5r" THEN tail5r(nowpos) = CVI(B$)
IF a$ = "tail5d" THEN tail5d(nowpos) = CVI(B$)
IF a$ = "tail5l" THEN tail5l(nowpos) = CVI(B$)
IF a$ = "ru1" THEN ru1(nowpos) = CVI(B$)
IF a$ = "ru2" THEN ru2(nowpos) = CVI(B$)
IF a$ = "ru3" THEN ru3(nowpos) = CVI(B$)
IF a$ = "ru4" THEN ru4(nowpos) = CVI(B$)
IF a$ = "rd1" THEN rd1(nowpos) = CVI(B$)
IF a$ = "rd2" THEN rd2(nowpos) = CVI(B$)
IF a$ = "rd3" THEN rd3(nowpos) = CVI(B$)
IF a$ = "rd4" THEN rd4(nowpos) = CVI(B$)
IF a$ = "ld1" THEN ld1(nowpos) = CVI(B$)
IF a$ = "ld2" THEN ld2(nowpos) = CVI(B$)
IF a$ = "ld3" THEN ld3(nowpos) = CVI(B$)
IF a$ = "ld4" THEN ld4(nowpos) = CVI(B$)
IF a$ = "lu1" THEN lu1(nowpos) = CVI(B$)
IF a$ = "lu2" THEN lu2(nowpos) = CVI(B$)
IF a$ = "lu3" THEN lu3(nowpos) = CVI(B$)
IF a$ = "lu4" THEN lu4(nowpos) = CVI(B$)
IF a$ = "hul1" THEN hul1(nowpos) = CVI(B$)
IF a$ = "hul2" THEN hul2(nowpos) = CVI(B$)
IF a$ = "hul3" THEN hul3(nowpos) = CVI(B$)
IF a$ = "hul4" THEN hul4(nowpos) = CVI(B$)
IF a$ = "hul5" THEN hul5(nowpos) = CVI(B$)
IF a$ = "hur1" THEN hur1(nowpos) = CVI(B$)
IF a$ = "hur2" THEN hur2(nowpos) = CVI(B$)
IF a$ = "hur3" THEN hur3(nowpos) = CVI(B$)
IF a$ = "hur4" THEN hur4(nowpos) = CVI(B$)
IF a$ = "hur5" THEN hur5(nowpos) = CVI(B$)
IF a$ = "hdr1" THEN hdr1(nowpos) = CVI(B$)
IF a$ = "hdr2" THEN hdr2(nowpos) = CVI(B$)
IF a$ = "hdr3" THEN hdr3(nowpos) = CVI(B$)
IF a$ = "hdr4" THEN hdr4(nowpos) = CVI(B$)
IF a$ = "hdr5" THEN hdr5(nowpos) = CVI(B$)
IF a$ = "hdl1" THEN hdl1(nowpos) = CVI(B$)
IF a$ = "hdl2" THEN hdl2(nowpos) = CVI(B$)
IF a$ = "hdl3" THEN hdl3(nowpos) = CVI(B$)
IF a$ = "hdl4" THEN hdl4(nowpos) = CVI(B$)
IF a$ = "hdl5" THEN hdl5(nowpos) = CVI(B$)
IF a$ = "hlu1" THEN hlu1(nowpos) = CVI(B$)
IF a$ = "hlu2" THEN hlu2(nowpos) = CVI(B$)
IF a$ = "hlu3" THEN hlu3(nowpos) = CVI(B$)
IF a$ = "hlu4" THEN hlu4(nowpos) = CVI(B$)
IF a$ = "hlu5" THEN hlu5(nowpos) = CVI(B$)
IF a$ = "hru1" THEN hru1(nowpos) = CVI(B$)
IF a$ = "hru2" THEN hru2(nowpos) = CVI(B$)
IF a$ = "hru3" THEN hru3(nowpos) = CVI(B$)
IF a$ = "hru4" THEN hru4(nowpos) = CVI(B$)
IF a$ = "hru5" THEN hru5(nowpos) = CVI(B$)
IF a$ = "hrd1" THEN hrd1(nowpos) = CVI(B$)
IF a$ = "hrd2" THEN hrd2(nowpos) = CVI(B$)
IF a$ = "hrd3" THEN hrd3(nowpos) = CVI(B$)
IF a$ = "hrd4" THEN hrd4(nowpos) = CVI(B$)
IF a$ = "hrd5" THEN hrd5(nowpos) = CVI(B$)
IF a$ = "hld1" THEN hld1(nowpos) = CVI(B$)
IF a$ = "hld2" THEN hld2(nowpos) = CVI(B$)
IF a$ = "hld3" THEN hld3(nowpos) = CVI(B$)
IF a$ = "hld4" THEN hld4(nowpos) = CVI(B$)
IF a$ = "hld5" THEN hld5(nowpos) = CVI(B$)
IF a$ = "tul1" THEN tul1(nowpos) = CVI(B$)
IF a$ = "tul2" THEN tul2(nowpos) = CVI(B$)
IF a$ = "tur1" THEN tur1(nowpos) = CVI(B$)
IF a$ = "tur2" THEN tur2(nowpos) = CVI(B$)
IF a$ = "tdr1" THEN tdr1(nowpos) = CVI(B$)
IF a$ = "tdr2" THEN tdr2(nowpos) = CVI(B$)
IF a$ = "tdl1" THEN tdl1(nowpos) = CVI(B$)
IF a$ = "tdl2" THEN tdl2(nowpos) = CVI(B$)
IF a$ = "tlu1" THEN tlu1(nowpos) = CVI(B$)
IF a$ = "tlu2" THEN tlu2(nowpos) = CVI(B$)
IF a$ = "tru1" THEN tru1(nowpos) = CVI(B$)
IF a$ = "tru2" THEN tru2(nowpos) = CVI(B$)
IF a$ = "trd1" THEN trd1(nowpos) = CVI(B$)
IF a$ = "trd2" THEN trd2(nowpos) = CVI(B$)
IF a$ = "tld1" THEN tld1(nowpos) = CVI(B$)
IF a$ = "tld2" THEN tld2(nowpos) = CVI(B$)
IF a$ = "rabbit1" THEN rabbit1(nowpos) = CVI(B$)
IF a$ = "rabbit2" THEN rabbit2(nowpos) = CVI(B$)
IF a$ = "head5ur" THEN head5ur(nowpos) = CVI(B$)
IF a$ = "head5rr" THEN head5rr(nowpos) = CVI(B$)
IF a$ = "head5dr" THEN head5dr(nowpos) = CVI(B$)
IF a$ = "head5lr" THEN head5lr(nowpos) = CVI(B$)
IF a$ = "head6ur" THEN head6ur(nowpos) = CVI(B$)
IF a$ = "head6rr" THEN head6rr(nowpos) = CVI(B$)
IF a$ = "head6dr" THEN head6dr(nowpos) = CVI(B$)
IF a$ = "head6lr" THEN head6lr(nowpos) = CVI(B$)
IF a$ = "head7ur" THEN head7ur(nowpos) = CVI(B$)
IF a$ = "head7rr" THEN head7rr(nowpos) = CVI(B$)
IF a$ = "head7dr" THEN head7dr(nowpos) = CVI(B$)
IF a$ = "head7lr" THEN head7lr(nowpos) = CVI(B$)
IF a$ = "hul4r" THEN hul4r(nowpos) = CVI(B$)
IF a$ = "hul5r" THEN hul5r(nowpos) = CVI(B$)
IF a$ = "hur4r" THEN hur4r(nowpos) = CVI(B$)
IF a$ = "hur5r" THEN hur5r(nowpos) = CVI(B$)
IF a$ = "hdr4r" THEN hdr4r(nowpos) = CVI(B$)
IF a$ = "hdr5r" THEN hdr5r(nowpos) = CVI(B$)
IF a$ = "hdl4r" THEN hdl4r(nowpos) = CVI(B$)
IF a$ = "hdl5r" THEN hdl5r(nowpos) = CVI(B$)
IF a$ = "hlu4r" THEN hlu4r(nowpos) = CVI(B$)
IF a$ = "hlu5r" THEN hlu5r(nowpos) = CVI(B$)
IF a$ = "hru4r" THEN hru4r(nowpos) = CVI(B$)
IF a$ = "hru5r" THEN hru5r(nowpos) = CVI(B$)
IF a$ = "hrd4r" THEN hrd4r(nowpos) = CVI(B$)
IF a$ = "hrd5r" THEN hrd5r(nowpos) = CVI(B$)
IF a$ = "hld4r" THEN hld4r(nowpos) = CVI(B$)
IF a$ = "hld5r" THEN hld5r(nowpos) = CVI(B$)
NEXT nowpos
CLOSE #2
LINE (236 + num, 271)-(236 + num, 289), 10
num = num + 1
WEND
CLOSE #1
CHDIR ".."
GOSUB 1100
PALETTE 0, 0: SCREEN 13: CLS
OPEN "piton.cfg" FOR INPUT AS #1
INPUT #1, gs&
CLOSE #1
5 FOR j = 0 TO 24
FOR i = 0 TO 39
PUT (i * 8, j * 8), grass, PSET
NEXT i, j
PUT (24, 32), rabbit1, PSET
PUT (176, 32), head1l, PSET
PUT (184, 32), hor3, PSET
PUT (192, 32), hor3, PSET
PUT (200, 32), hor3, PSET
PUT (208, 32), hor3, PSET
PUT (216, 32), hor3, PSET
PUT (224, 32), hor3, PSET
PUT (232, 32), hor3, PSET
PUT (240, 32), hor3, PSET
PUT (248, 32), ru1, PSET
PUT (248, 40), ver3, PSET
PUT (248, 48), ver3, PSET
PUT (248, 56), ver3, PSET
PUT (248, 64), tail1u, PSET
LINE (2, 2)-(317, 197), 8, B
LINE (3, 3)-(316, 196), 8, B
LINE (4, 4)-(315, 195), 8, B
LINE (5, 5)-(314, 194), 8, B
LINE (101, 61)-(203, 123), 14, B
LINE (102, 62)-(202, 122), 1, B
LINE (103, 63)-(201, 121), 1, B
LINE (104, 64)-(200, 120), 14, B
LINE (105, 65)-(199, 119), 0, BF
LOCATE 10, 17: COLOR 12: PRINT "PITON"
menu$(1) = "NEW GAME"
menu$(2) = "DEMO"
menu$(3) = "GAME SPEED"
rcursor = 1
FOR f = 1 TO 5
IF f = 1 THEN COLOR 15 ELSE COLOR 3
LOCATE 11 + f, 15: PRINT menu$(f)
NEXT f
10 i$ = INKEY$
IF i$ = "" THEN GOTO 10
IF ASC(i$) = 27 THEN COLOR 7: CLS : SCREEN 0: WIDTH 80: END
IF ASC(i$) = 13 OR i$ = " " THEN ON rcursor GOTO 20, 20, 1300
IF LEN(i$) = 1 THEN GOTO 10
i = ASC(RIGHT$(i$, 1))
IF i = 80 THEN LOCATE 11 + rcursor, 15: COLOR 3: PRINT menu$(rcursor): rcursor = rcursor + 1: IF rcursor = 4 THEN rcursor = 1
IF i = 72 THEN LOCATE 11 + rcursor, 15: COLOR 3: PRINT menu$(rcursor): rcursor = rcursor - 1: IF rcursor = 0 THEN rcursor = 3
IF i = 80 OR i = 72 THEN LOCATE 11 + rcursor, 15: COLOR 15: PRINT menu$(rcursor)
GOTO 10
20 IF rcursor = 1 THEN demo = 0 ELSE demo = 1
FOR j = 0 TO 24
FOR i = 0 TO 39
PUT (i * 8, j * 8), grass, PSET
NEXT i, j
LINE (2, 2)-(317, 197), 8, B
LINE (3, 3)-(316, 196), 8, B
LINE (4, 4)-(315, 195), 8, B
LINE (5, 5)-(314, 194), 8, B
FOR j = 1 TO 23
FOR i = 1 TO 38
ml(i, j) = 0
NEXT i, j
ml(18, 23) = 22
ml(19, 23) = 2
ml(20, 23) = 14
n = 2: hx = 20: hy = 23: tx = 18: ty = 23
GOSUB 1500
30 IF hx = rx AND hy = ry THEN ntail = 1: GOSUB 1500 ELSE ntail = 0
IF l = 750 THEN CLS : COLOR 12: LOCATE 13, 13: PRINT "YOU ARE WIN!!!": WHILE INKEY$ = "": WEND
IF demo = 1 THEN GOTO 40
a$ = INKEY$
IF a$ <> "" THEN i$ = a$: GOTO 30
IF ASC(i$) = 27 THEN GOTO 5
IF LEN(i$) = 1 THEN GOTO 50
i = ASC(RIGHT$(i$, 1))
IF i = 72 AND n <> 3 THEN n = 1
IF i = 77 AND n <> 4 THEN n = 2
IF i = 80 AND n <> 1 THEN n = 3
IF i = 75 AND n <> 2 THEN n = 4
GOTO 50
40 IF INKEY$ <> "" THEN GOTO 5
IF rx - hx > 0 AND ry - hy > 0 AND n = 4 THEN n = 3: GOTO 50
IF rx - hx > 0 AND ry - hy > 0 AND n = 1 THEN n = 2: GOTO 50
IF rx - hx < 0 AND ry - hy > 0 AND n = 1 THEN n = 4: GOTO 50
IF rx - hx < 0 AND ry - hy > 0 AND n = 2 THEN n = 3: GOTO 50
IF rx - hx > 0 AND ry - hy < 0 AND n = 3 THEN n = 2: GOTO 50
IF rx - hx > 0 AND ry - hy < 0 AND n = 4 THEN n = 1: GOTO 50
IF rx - hx < 0 AND ry - hy < 0 AND n = 3 THEN n = 4: GOTO 50
IF rx - hx < 0 AND ry - hy < 0 AND n = 2 THEN n = 1: GOTO 50
IF rx - hx = 0 AND ry - hy > 0 AND n <> 1 THEN n = 3: GOTO 50
IF rx - hx = 0 AND ry - hy < 0 AND n <> 3 THEN n = 1: GOTO 50
IF rx - hx > 0 AND ry - hy = 0 AND n <> 4 THEN n = 2: GOTO 50
IF rx - hx < 0 AND ry - hy = 0 AND n <> 2 THEN n = 4: GOTO 50
IF n = 1 OR n = 3 THEN n = 2: GOTO 50
IF n = 2 OR n = 4 THEN n = 1: GOTO 50
50 IF n = 1 THEN hnx = hx: hny = hy - 1
IF n = 2 THEN hnx = hx + 1: hny = hy
IF n = 3 THEN hnx = hx: hny = hy + 1
IF n = 4 THEN hnx = hx - 1: hny = hy
IF hnx = 0 OR hnx = 39 OR hny = 0 OR hny = 24 THEN CLS : COLOR 12: LOCATE 13, 15: PRINT "GAME OVER": WHILE INKEY$ = "": WEND
IF ml(hnx, hny) <> 0 THEN CLS : COLOR 12: LOCATE 13, 15: PRINT "GAME OVER": WHILE INKEY$ = "": WEND
IF hnx = rx AND hny = ry THEN rabbit = 1: l = l + 1 ELSE rabbit = 0
IF ml(hx, hy) = 13 AND n = 1 THEN ml(hnx, hny) = 13: ml(tx, ty) = 17
IF ml(hx, hy) = 13 AND n = 2 THEN ml(hnx, hny) = 32: ml(tx, ty) = 40
IF ml(hx, hy) = 13 AND n = 4 THEN ml(hnx, hny) = 25: ml(tx, ty) = 33
IF ml(hx, hy) = 14 AND n = 1 THEN ml(hnx, hny) = 28: ml(tx, ty) = 36
IF ml(hx, hy) = 14 AND n = 2 THEN ml(hnx, hny) = 14: ml(tx, ty) = 18
IF ml(hx, hy) = 14 AND n = 3 THEN ml(hnx, hny) = 29: ml(tx, ty) = 37
IF ml(hx, hy) = 15 AND n = 2 THEN ml(hnx, hny) = 27: ml(tx, ty) = 35
IF ml(hx, hy) = 15 AND n = 3 THEN ml(hnx, hny) = 15: ml(tx, ty) = 19
IF ml(hx, hy) = 15 AND n = 4 THEN ml(hnx, hny) = 30: ml(tx, ty) = 38
IF ml(hx, hy) = 16 AND n = 1 THEN ml(hnx, hny) = 31: ml(tx, ty) = 39
IF ml(hx, hy) = 16 AND n = 3 THEN ml(hnx, hny) = 26: ml(tx, ty) = 34
IF ml(hx, hy) = 16 AND n = 4 THEN ml(hnx, hny) = 16: ml(tx, ty) = 20
IF ml(tx, ty) = 21 THEN tnx = tx: tny = ty - 1:
IF ml(tx, ty) = 22 THEN tnx = tx + 1: tny = ty:
IF ml(tx, ty) = 23 THEN tnx = tx: tny = ty + 1:
IF ml(tx, ty) = 24 THEN tnx = tx - 1: tny = ty:
IF ml(tx, ty) = 21 AND ml(tnx, tny) = 1 THEN ml(tnx, tny) = 21: ml(tx, ty) = 57
IF ml(tx, ty) = 21 AND ml(tnx, tny) = 5 THEN ml(tnx, tny) = 41: ml(tx, ty) = 57
IF ml(tx, ty) = 21 AND ml(tnx, tny) = 12 THEN ml(tnx, tny) = 48: ml(tx, ty) = 57



'Подпрограмма очистки буфера клавиатуры
1000 FOR f = 1 TO 20
i$ = INKEY$
NEXT f
RETURN

'Подпрограмма 10-секундной паузы или нажатия любой клавиши
1100 fl = 0
t = ASC(RIGHT$(TIME$, 1))
FOR f = 1 TO 11
1110 IF INKEY$ <> "" THEN fl = 1
IF t = ASC(RIGHT$(TIME$, 1)) AND fl = 0 THEN GOTO 1110
t = ASC(RIGHT$(TIME$, 1))
NEXT f
RETURN

'Подпрограмма исправления ошибки(нет ненужного файла)
1200 RESUME NEXT

'Подпрограмма выбора скорости задержки игры
1300 LINE (37, 149)-(259, 187), 14, B
LINE (38, 150)-(258, 186), 1, B
LINE (39, 151)-(257, 185), 1, B
LINE (40, 152)-(256, 184), 14, B
LINE (41, 153)-(255, 183), 0, BF
COLOR 10: LOCATE 21, 7: PRINT "OLD GAME SPEED:"; RIGHT$(STR$(gs&), LEN(STR$(gs&)) - 1)
COLOR 12: LOCATE 22, 7: PRINT "NEW GAME SPEED:"; : COLOR 10
PRINT "_"; CHR$(29);
a$ = ""
1310 i$ = INKEY$
IF i$ = "" THEN GOTO 1310
IF ASC(i$) = 27 THEN GOSUB 1400: GOTO 10
IF ASC(i$) = 8 AND LEN(a$) > 0 THEN a$ = LEFT$(a$, LEN(a$) - 1): PRINT " "; CHR$(29); CHR$(29); "_"; CHR$(29);
IF ASC(i$) > 47 AND ASC(i$) < 58 AND LEN(a$) < 9 THEN a$ = a$ + i$: PRINT i$; "_"; CHR$(29);
IF ASC(i$) <> 13 THEN GOTO 1310
a& = VAL(a$)
IF a = 0 THEN GOSUB 1400: GOTO 10
gs& = a&
OPEN "piton.cfg" FOR OUTPUT AS #1
PRINT #1, gs&
CLOSE #1
GOSUB 1400: GOTO 10

'Подпрограмма очистки нижней части экрана
1400 FOR j = 18 TO 23
FOR i = 1 TO 38
PUT (i * 8, j * 8), grass, PSET
NEXT i, j
RETURN

'Подпрограмма появления кролика
1500 rx = INT(RND(1) * 38 + 1)
ry = INT(RND(1) * 23 + 1)
IF ml(x, y) > 0 THEN GOTO 1500
PUT (x * 8, y * 8), rabbit1
RETURN

