DEFINT A-Z
SCREEN 12: CLS
PALETTE 0, 2039552
PALETTE 6, 12336
DRAW "c9bm10,10m60,170r30m140,10l20m75,150m30,10l20bm160,40u30r120d30bm-10,-10l40d110bm+10,+10d20l40u20bm+10,-10u110l40"
CIRCLE (170, 40), 10, 9, 1.57, 3.14
CIRCLE (270, 40), 10, 9, 0, 1.57
CIRCLE (200, 140), 10, 9, 4.71, 0
CIRCLE (240, 140), 10, 9, 3.14, 4.71
PAINT (75, 160), 14, 9
PAINT (220, 20), 14, 9
f = 0
FOR j = 11 TO 170
FOR i = 1 TO 281
f = f + 1
IF f = 4 THEN f = 0
IF f = 1 THEN IF POINT(i, j) = 14 THEN PSET (i, j), 6
IF f = 1 THEN IF POINT(280 - i, j) = 14 THEN PSET (280 - i, j), 6
NEXT i, j
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
LOCATE 24, 27: PRINT "Russia, Klintsy, 2001 a.d."
LOCATE 26, 30: PRINT "tel: (08336) 2-39-91"
LOCATE 27, 35: PRINT "(08336) 2-00-21"
GOSUB 1500
GOSUB 1600
PALETTE 0, 0: CLS
DIM m&(256), m1&(256), m2&(256), m3&(256)
OPEN "palette.dat" FOR RANDOM AS #1 LEN = 4
FIELD #1, 4 AS a$
FOR f = 1 TO 256
GET #1, f
m&(f) = CVL(a$)
a& = m&(f)
m1&(f) = 0: m2&(f) = 0: m3&(f) = 0
WHILE a& > 65535
m1&(f) = m1&(f) + 1
a& = a& - 65536
WEND
WHILE a& > 255
m2&(f) = m2&(f) + 1
a& = a& - 256
WEND
m3&(f) = a&
NEXT f
CLOSE #1
SCREEN 13: CLS
PALETTE USING m&(1)
COLOR 15
FOR f = 0 TO 128 STEP 8
LINE (96, f)-(224, f), 15
LINE (96 + f, 0)-(96 + f, 128), 15
NEXT f
LINE (0, 0)-(65, 65), 15, B
LINE (254, 0)-(319, 65), 15, B
FOR j = 0 TO 15
FOR i = 0 TO 15
LINE (97 + i * 8, 1 + j * 8)-(103 + i * 8, 7 + j * 8), j * 16 + i, BF
LINE (1 + i * 4, 1 + j * 4)-(4 + i * 4, 4 + j * 4), j * 16 + i, BF
NEXT i, j
LOCATE 18, 1: PRINT "BLUE";
LOCATE 20, 1: PRINT "GREEN";
LOCATE 22, 1: PRINT "RED";
LINE (48, 136)-(241, 142), 15, B
LINE (48, 152)-(241, 158), 15, B
LINE (48, 168)-(241, 174), 15, B
LINE (96, 0)-(104, 8), 10, B
LINE (255, 1)-(318, 64), 0, BF
x = 0: y = 0: n = 1: z = 1
GOSUB 1100
LINE (48, 120 + n * 16)-(241, 126 + n * 16), 10, B
10 i$ = INKEY$
IF LEN(i$) = 0 THEN GOTO 10
IF ASC(i$) = 27 THEN GOTO 1000
IF i$ = "/" THEN LINE (48, 120 + n * 16)-(241, 126 + n * 16), 15, B: n = n + 1
IF n = 4 THEN n = 1
IF i$ = "/" THEN LINE (48, 120 + n * 16)-(241, 126 + n * 16), 10, B
IF i$ = "," THEN GOSUB 1200
IF i$ = "." THEN GOSUB 1300
IF LEN(i$) = 1 THEN GOTO 10
i = ASC(RIGHT$(i$, 1))
IF i = 72 OR i = 80 OR i = 75 OR i = 77 THEN GOSUB 1400
GOTO 10


1000 LOCATE 24, 10: PRINT "Save?[Yes/No/Cancel]";
1010 i$ = INKEY$
IF LEN(i$) = 0 THEN GOTO 1010
IF ASC(i$) = 27 OR i$ = "c" OR i$ = "C" THEN LOCATE 24, 10: PRINT SPACE$(20); : GOTO 10
IF i$ = "n" OR i$ = "N" THEN SCREEN 0: WIDTH 80: COLOR 7: CLS : END
IF i$ = "y" OR i$ = "Y" THEN GOTO 1020
GOTO 1010
1020 OPEN "palette.dat" FOR RANDOM AS #1 LEN = 4
FIELD #1, 4 AS a$
FOR f = 1 TO 256
LSET a$ = MKL$(m&(f))
PUT #1, f
NEXT f
CLOSE #1
SCREEN 0: WIDTH 80: COLOR 7: CLS : END

1100 LINE (49, 137)-(240, 141), 0, BF
LINE (49, 153)-(240, 157), 0, BF
LINE (49, 169)-(240, 173), 0, BF
LINE (49 + m1&(z) * 3, 137)-(51 + m1&(z) * 3, 141), 15, BF
LINE (49 + m2&(z) * 3, 153)-(51 + m2&(z) * 3, 157), 15, BF
LINE (49 + m3&(z) * 3, 169)-(51 + m3&(z) * 3, 173), 15, BF
LOCATE 18, 32: PRINT m1&(z);
LOCATE 20, 32: PRINT m2&(z);
LOCATE 22, 32: PRINT m3&(z);
RETURN

1200 IF n = 1 THEN m1&(z) = m1&(z) - 1
IF m1&(z) = -1 THEN m1&(z) = 63
IF n = 2 THEN m2&(z) = m2&(z) - 1
IF m2&(z) = -1 THEN m2&(z) = 63
IF n = 3 THEN m3&(z) = m3&(z) - 1
IF m3&(z) = -1 THEN m3&(z) = 63
LINE (49, 121 + n * 16)-(240, 125 + n * 16), 0, BF
IF n = 1 THEN LINE (49 + m1&(z) * 3, 137)-(51 + m1&(z) * 3, 141), 15, BF: LOCATE 18, 32: PRINT m1&(z);
IF n = 2 THEN LINE (49 + m2&(z) * 3, 153)-(51 + m2&(z) * 3, 157), 15, BF: LOCATE 20, 32: PRINT m2&(z);
IF n = 3 THEN LINE (49 + m3&(z) * 3, 169)-(51 + m3&(z) * 3, 173), 15, BF: LOCATE 22, 32: PRINT m3&(z);
m&(z) = m1&(z) * 65536 + m2&(z) * 256 + m3&(z)
PALETTE z - 1, m&(z)
RETURN

1300 IF n = 1 THEN m1&(z) = m1&(z) + 1
IF m1&(z) = 64 THEN m1&(z) = 0
IF n = 2 THEN m2&(z) = m2&(z) + 1
IF m2&(z) = 64 THEN m2&(z) = 0
IF n = 3 THEN m3&(z) = m3&(z) + 1
IF m3&(z) = 64 THEN m3&(z) = 0
LINE (49, 121 + n * 16)-(240, 125 + n * 16), 0, BF
IF n = 1 THEN LINE (49 + m1&(z) * 3, 137)-(51 + m1&(z) * 3, 141), 15, BF: LOCATE 18, 32: PRINT m1&(z);
IF n = 2 THEN LINE (49 + m2&(z) * 3, 153)-(51 + m2&(z) * 3, 157), 15, BF: LOCATE 20, 32: PRINT m2&(z);
IF n = 3 THEN LINE (49 + m3&(z) * 3, 169)-(51 + m3&(z) * 3, 173), 15, BF: LOCATE 22, 32: PRINT m3&(z);
m&(z) = m1&(z) * 65536 + m2&(z) * 256 + m3&(z)
PALETTE z - 1, m&(z)
RETURN

1400 LINE (96 + x * 8, y * 8)-(104 + x * 8, y * 8 + 8), 15, B
IF i = 72 THEN y = y - 1
IF y = -1 THEN y = 15
IF i = 80 THEN y = y + 1
IF y = 16 THEN y = 0
IF i = 75 THEN x = x - 1
IF x = -1 THEN x = 15
IF i = 77 THEN x = x + 1
IF x = 16 THEN x = 0
LINE (96 + x * 8, y * 8)-(104 + x * 8, y * 8 + 8), 10, B
LINE (255, 1)-(318, 64), y * 16 + x, BF
z = y * 16 + x + 1
GOSUB 1100
RETURN

'Подпрограмма очистки буфера клавиатуры
1500 FOR f = 1 TO 20
i$ = INKEY$
NEXT f
RETURN

'Подпрограмма 10-секундной паузы или нажатия любой клавиши
1600 fl = 0
t = ASC(RIGHT$(TIME$, 1))
FOR f = 1 TO 11
1610 IF INKEY$ <> "" THEN fl = 1
IF t = ASC(RIGHT$(TIME$, 1)) AND fl = 0 THEN GOTO 1610
t = ASC(RIGHT$(TIME$, 1))
NEXT f
RETURN

