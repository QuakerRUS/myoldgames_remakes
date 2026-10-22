DEFINT A-Z
DIM pal&(256), spr(19500)
'CHDIR ".."
OPEN "palette.dat" FOR RANDOM AS #1 LEN = 4
'CHDIR "EDITOR"
FIELD #1, 4 AS a$
FOR f = 1 TO 256
GET #1, f
pal&(f) = CVL(a$)
NEXT f
CLOSE #1

GOTO 1

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
GOSUB 10000
GOSUB 10100
1 PALETTE 0, 0: CLS
SCREEN 13: CLS
PALETTE USING pal&(1)
COLOR 16: LOCATE 1, 9: PRINT "EDITOR FOR SOLDIER v2.0"
FOR j = 0 TO 6
FOR i = 55 TO 265
IF POINT(i, j) = 16 THEN PSET (i, j), 63 - j
NEXT i, j
cur1 = 1
10 LOCATE 10, 14: PRINT "SPRITE EDITOR"
LOCATE 11, 14: PRINT "OBJECT EDITOR"
LOCATE 12, 14: PRINT "LEVEL EDITOR"
LOCATE 13, 14: PRINT "EPISODE EDITOR"
FOR f = 9 TO 12
FOR j = 0 TO 6
y = f * 8 + j
FOR i = 104 TO 216
IF POINT(i, y) = 16 THEN PSET (i, y), 127 - j
NEXT i, j, f
FOR j = 71 TO 79
FOR i = 103 TO 215
IF POINT(i, j) = 0 THEN PSET (i, j), 118 - j
NEXT i, j
20 ink$ = INKEY$
IF LEN(ink$) = 0 THEN GOTO 20
IF ASC(ink$) = 27 THEN SCREEN 0: WIDTH 80: COLOR 7: CLS : END
IF ink$ = " " OR ASC(ink$) = 13 THEN LINE (103, 70)-(215, 106), 0, BF: ON cur1 GOTO 1000, 2000, 3000, 4000
IF LEN(ink$) = 1 THEN GOTO 20
ink = ASC(RIGHT$(ink$, 1))
IF ink = 72 THEN
        FOR j = -1 TO 7
        FOR i = 103 TO 215
        y = (cur1 + 8) * 8 + j
        IF POINT(i, y) < 100 THEN PSET (i, y), 0
        NEXT i, j
        cur1 = cur1 - 1
        IF cur1 = 0 THEN cur1 = 4
        FOR j = -1 TO 7
        FOR i = 103 TO 215
        y = (cur1 + 8) * 8 + j
        IF POINT(i, y) = 0 THEN PSET (i, y), 46 - j
        NEXT i, j
END IF
IF ink = 80 THEN
        FOR j = -1 TO 7
        FOR i = 103 TO 215
        y = (cur1 + 8) * 8 + j
        IF POINT(i, y) < 100 THEN PSET (i, y), 0
        NEXT i, j
        cur1 = cur1 + 1
        IF cur1 = 5 THEN cur1 = 1
        FOR j = -1 TO 7
        FOR i = 103 TO 215
        y = (cur1 + 8) * 8 + j
        IF POINT(i, y) = 0 THEN PSET (i, y), 46 - j
        NEXT i, j
END IF
GOTO 20
END

'Редактор спрайтов
1000 x1 = 0
y1 = 0
x2 = 0
y2 = 0
col = 0
pcol = 0
fl = 0
GOSUB 10200
FOR f = 0 TO 128 STEP 8
LINE (0, 16 + f)-(128, 16 + f), 15
LINE (f, 16)-(f, 144), 15
LINE (191, 16 + f)-(319, f + 16), 15
LINE (191 + f, 16)-(191 + f, 144), 15
NEXT f
LINE (0, 146)-(17, 163), 15, B
LINE (302, 146)-(319, 163), 15, B
COLOR 16: LOCATE 20, 4: PRINT "TEXTURE"
LOCATE 20, 31: PRINT "PALETTE"
FOR j = 0 TO 6
FOR i = 24 TO 80
IF POINT(i, j + 152) = 16 THEN PSET (i, j + 152), 127 - j
IF POINT(i + 216, j + 152) = 16 THEN PSET (i + 216, j + 152), 127 - j
NEXT i, j
FOR j = 0 TO 15
FOR i = 0 TO 15
PAINT (195 + i * 8, 21 + j * 8), j * 16 + i, 15
NEXT i, j
LINE (0, 16)-(8, 24), 10, B
ON KEY(1) GOSUB 10200
1010 GOSUB 10400
ink$ = INKEY$
IF ink$ = "" THEN GOTO 1010
GOSUB 10300
IF fl = 0 THEN LINE (x1 * 8, y1 * 8 + 16)-(x1 * 8 + 8, y1 * 8 + 24), 15, B ELSE LINE (x2 * 8 + 191, y2 * 8 + 16)-(x2 * 8 + 199, y2 * 8 + 24), 15, B
IF ASC(ink$) = 27 THEN LINE (0, 16)-(319, 199), 0, BF: GOTO 10
IF ASC(ink$) = 9 THEN fl = NOT (fl)
IF (ink$ = " " OR ASC(ink$) = 13) AND fl = 0 THEN lev(x1 + 1, y1 + 1) = col: LINE (x1 * 8 + 1, y1 * 8 + 17)-(x1 * 8 + 7, y1 * 8 + 23), col, BF: PSET (1 + x1, 147 + y1), col
IF ASC(ink$) = 8 AND fl = 0 THEN lev(x1 + 1, y1 + 1) = pcol: LINE (x1 * 8 + 1, y1 * 8 + 17)-(x1 * 8 + 7, y1 * 8 + 23), pcol, BF: PSET (1 + x1, 147 + y1), pcol
IF LEN(ink$) = 1 THEN GOSUB 10600: GOTO 1010
ink = ASC(RIGHT$(ink$, 1))
IF ink = 75 AND fl = 0 THEN x1 = x1 - 1
IF ink = 80 AND fl = 0 THEN y1 = y1 + 1
IF ink = 77 AND fl = 0 THEN x1 = x1 + 1
IF ink = 72 AND fl = 0 THEN y1 = y1 - 1
IF ink = 75 AND fl = -1 THEN x2 = x2 - 1
IF ink = 80 AND fl = -1 THEN y2 = y2 + 1
IF ink = 77 AND fl = -1 THEN x2 = x2 + 1
IF ink = 72 AND fl = -1 THEN y2 = y2 - 1
IF x1 = -1 THEN x1 = 15
IF x1 = 16 THEN x1 = 0
IF y1 = -1 THEN y1 = 15
IF y1 = 16 THEN y1 = 0
IF x2 = -1 THEN x2 = 15
IF x2 = 16 THEN x2 = 0
IF y2 = -1 THEN y2 = 15
IF y2 = 16 THEN y2 = 0
col = y2 * 16 + x2
LINE (303, 147)-(318, 162), col, BF
GOSUB 10600
GOTO 1010

'Редактор объектов
2000 END

'Редактор уровней
3000 END

'Редактор эпизодов
4000 END



'Подпрограмма очистки буфера клавиатуры
10000 FOR f = 1 TO 20
i$ = INKEY$
NEXT f
RETURN

'Подпрограмма 10-секундной паузы или нажатия любой клавиши
10100 fl = 0
t = ASC(RIGHT$(TIME$, 1))
FOR f = 1 TO 11
10110 IF INKEY$ <> "" THEN fl = 1
IF t = ASC(RIGHT$(TIME$, 1)) AND fl = 0 THEN GOTO 10110
t = ASC(RIGHT$(TIME$, 1))
NEXT f
RETURN

'Подпрограмма закрашивания спрайта в редакторе спрайтов
10200 GOSUB 10300
FOR j = 1 TO 16
FOR i = 1 TO 16
pcol = col
lev(i, j) = pcol
NEXT i, j
GOSUB 10700
RETURN

'Подпрограмма временного отключения функциональных клавиш в редакторе спрайтов
10300 KEY(1) STOP
KEY(2) STOP
KEY(3) STOP
KEY(4) STOP
KEY(5) STOP
KEY(6) STOP
KEY(7) STOP
KEY(8) STOP
KEY(9) STOP
KEY(10) STOP
RETURN

'Подпрограмма включения функциональных клавиш в редакторе спрайтов
10400 KEY(1) ON
KEY(2) ON
KEY(3) ON
KEY(4) ON
KEY(5) ON
KEY(6) ON
KEY(7) ON
KEY(8) ON
KEY(9) ON
KEY(10) ON
RETURN

'Подпрограмма выключения функциональных клавиш в редакторе спрайтов
10500 KEY(1) OFF
KEY(2) OFF
KEY(3) OFF
KEY(4) OFF
KEY(5) OFF
KEY(6) OFF
KEY(7) OFF
KEY(8) OFF
KEY(9) OFF
KEY(10) OFF
RETURN

'Подпрограмма рисования курсора в редакторе спрайтов
10600 IF fl = 0 THEN LINE (x1 * 8, y1 * 8 + 16)-(x1 * 8 + 8, y1 * 8 + 24), 10, B ELSE LINE (x2 * 8 + 191, y2 * 8 + 16)-(x2 * 8 + 199, y2 * 8 + 24), 10, B
RETURN

'Подпрограмма высвечивания текстуры
10700 FOR j = 0 TO 15
FOR i = 0 TO 15
LINE (i * 8 + 1, j * 8 + 17)-(i * 8 + 7, j * 8 + 23), lev(i + 1, j + 1), BF
PSET (i + 1, 147 + j), lev(i + 1, j + 1)
NEXT i, j
RETURN

