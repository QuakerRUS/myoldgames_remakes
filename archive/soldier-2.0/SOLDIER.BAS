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
GOSUB 1000
GOSUB 1100
PALETTE 0, 0: CLS

END


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

