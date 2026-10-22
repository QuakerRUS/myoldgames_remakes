DEFINT A-Z
DIM ml(100, 100), inv(25)
FOR j = 0 TO 4
FOR i = 0 TO 4
inv(j * 5 + i + 1) = 0
NEXT i, j
rand = VAL(LEFT$(TIME$, 2)) * 1000 + VAL(MID$(TIME$, 4, 2)) * 100 + VAL(RIGHT$(TIME$, 2))
ON ERROR GOTO 1000
OPEN "soldier.tmp" FOR INPUT AS #1
ON ERROR GOTO 0
INPUT #1, game
INPUT #1, lcursor
INPUT #1, level
INPUT #1, gs&
lcursor$ = RIGHT$(STR$(lcursor), LEN(STR$(lcursor)) - 1)
level$ = RIGHT$(STR$(level), LEN(STR$(level)) - 1)
CLOSE #1
KILL "soldier.tmp"
CHDIR "RECORDS"
ON ERROR GOTO 1890
KILL "record.tmp"
ON ERROR GOTO 0
CHDIR ".."
SCREEN 13: CLS
DIM floor1(129), stone1(129), exit1(129), player1d(129), player1u(129), player1l(129), player1r(129), die1(129), die2(129), box1(129), switch1(129), door1(129), redkey1(129), reddoor1(129), grkey1(129), grdoor1(129), blkey1(129), bldoor1(129),  _
yelkey1(129), yeldoor1(129), whkey1(129), whdoor1(129), blkkey1(129), blkdoor1(129), darkst1(129), hilotin1(129), hilotin2(129), player2d(129), player2u(129), player2l(129), player2r(129), die3(129), water1(129), water2(129), water3(129), water4(129 _
), die4(129), floor2(129), player3d(129), player3u(129), player3l(129), player3r(129), die5(129), die6(129)
GET (0, 0)-(15, 15), floor1
GET (0, 0)-(15, 15), stone1
GET (0, 0)-(15, 15), exit1
GET (0, 0)-(15, 15), player1d
GET (0, 0)-(15, 15), player1u
GET (0, 0)-(15, 15), player1l
GET (0, 0)-(15, 15), player1r
GET (0, 0)-(15, 15), die1
GET (0, 0)-(15, 15), die2
GET (0, 0)-(15, 15), box1
GET (0, 0)-(15, 15), switch1
GET (0, 0)-(15, 15), door1
GET (0, 0)-(15, 15), redkey1
GET (0, 0)-(15, 15), reddoor1
GET (0, 0)-(15, 15), grkey1
GET (0, 0)-(15, 15), grdoor1
GET (0, 0)-(15, 15), blkey1
GET (0, 0)-(15, 15), bldoor1
GET (0, 0)-(15, 15), yelkey1
GET (0, 0)-(15, 15), yeldoor1
GET (0, 0)-(15, 15), whkey1
GET (0, 0)-(15, 15), whdoor1
GET (0, 0)-(15, 15), blkkey1
GET (0, 0)-(15, 15), blkdoor1
GET (0, 0)-(15, 15), darkst1
GET (0, 0)-(15, 15), hilotin1
GET (0, 0)-(15, 15), hilotin2
GET (0, 0)-(15, 15), player2d
GET (0, 0)-(15, 15), player2u
GET (0, 0)-(15, 15), player2l
GET (0, 0)-(15, 15), player2r
GET (0, 0)-(15, 15), die3
GET (0, 0)-(15, 15), water1
GET (0, 0)-(15, 15), water2
GET (0, 0)-(15, 15), water3
GET (0, 0)-(15, 15), water4
GET (0, 0)-(15, 15), die4
GET (0, 0)-(15, 15), floor2
GET (0, 0)-(15, 15), player3d
GET (0, 0)-(15, 15), player3u
GET (0, 0)-(15, 15), player3l
GET (0, 0)-(15, 15), player3r
GET (0, 0)-(15, 15), die5
GET (0, 0)-(15, 15), die6
OPEN "graph1.dat" FOR INPUT AS #1
CHDIR "GRAPHICS"
WHILE EOF(1) <> -1
INPUT #1, a$
OPEN a$ + ".dat" FOR RANDOM AS #2 LEN = 2
FIELD #2, 2 AS b$
FOR nowpos = 1 TO 129
GET #2, nowpos
IF a$ = "floor1" THEN floor1(nowpos) = CVI(b$)
IF a$ = "stone1" THEN stone1(nowpos) = CVI(b$)
IF a$ = "exit1" THEN exit1(nowpos) = CVI(b$)
IF a$ = "player1d" THEN player1d(nowpos) = CVI(b$)
IF a$ = "player1u" THEN player1u(nowpos) = CVI(b$)
IF a$ = "player1l" THEN player1l(nowpos) = CVI(b$)
IF a$ = "player1r" THEN player1r(nowpos) = CVI(b$)
IF a$ = "die1" THEN die1(nowpos) = CVI(b$)
IF a$ = "die2" THEN die2(nowpos) = CVI(b$)
IF a$ = "box1" THEN box1(nowpos) = CVI(b$)
IF a$ = "switch1" THEN switch1(nowpos) = CVI(b$)
IF a$ = "door1" THEN door1(nowpos) = CVI(b$)
IF a$ = "redkey1" THEN redkey1(nowpos) = CVI(b$)
IF a$ = "reddoor1" THEN reddoor1(nowpos) = CVI(b$)
IF a$ = "grkey1" THEN grkey1(nowpos) = CVI(b$)
IF a$ = "grdoor1" THEN grdoor1(nowpos) = CVI(b$)
IF a$ = "blkey1" THEN blkey1(nowpos) = CVI(b$)
IF a$ = "bldoor1" THEN bldoor1(nowpos) = CVI(b$)
IF a$ = "yelkey1" THEN yelkey1(nowpos) = CVI(b$)
IF a$ = "yeldoor1" THEN yeldoor1(nowpos) = CVI(b$)
IF a$ = "whkey1" THEN whkey1(nowpos) = CVI(b$)
IF a$ = "whdoor1" THEN whdoor1(nowpos) = CVI(b$)
IF a$ = "blkkey1" THEN blkkey1(nowpos) = CVI(b$)
IF a$ = "blkdoor1" THEN blkdoor1(nowpos) = CVI(b$)
IF a$ = "darkst1" THEN darkst1(nowpos) = CVI(b$)
IF a$ = "hilotin1" THEN hilotin1(nowpos) = CVI(b$)
IF a$ = "hilotin2" THEN hilotin2(nowpos) = CVI(b$)
IF a$ = "player2d" THEN player2d(nowpos) = CVI(b$)
IF a$ = "player2u" THEN player2u(nowpos) = CVI(b$)
IF a$ = "player2l" THEN player2l(nowpos) = CVI(b$)
IF a$ = "player2r" THEN player2r(nowpos) = CVI(b$)
IF a$ = "die3" THEN die3(nowpos) = CVI(b$)
IF a$ = "water1" THEN water1(nowpos) = CVI(b$)
IF a$ = "water2" THEN water2(nowpos) = CVI(b$)
IF a$ = "water3" THEN water3(nowpos) = CVI(b$)
IF a$ = "water4" THEN water4(nowpos) = CVI(b$)
IF a$ = "die4" THEN die4(nowpos) = CVI(b$)
IF a$ = "floor2" THEN floor2(nowpos) = CVI(b$)
IF a$ = "player3d" THEN player3d(nowpos) = CVI(b$)
IF a$ = "player3u" THEN player3u(nowpos) = CVI(b$)
IF a$ = "player3l" THEN player3l(nowpos) = CVI(b$)
IF a$ = "player3r" THEN player3r(nowpos) = CVI(b$)
IF a$ = "die5" THEN die5(nowpos) = CVI(b$)
IF a$ = "die6" THEN die6(nowpos) = CVI(b$)
NEXT nowpos
CLOSE #2
WEND
CLOSE #1
CHDIR ".."
CHDIR "LEVELS"
OPEN "level" + level$ + ".dat" FOR RANDOM AS #1 LEN = 2
FIELD #1, 2 AS rec$
GET #1, 1
hor = CVI(rec$)
GET #1, 2
ver = CVI(rec$)
GET #1, 3
time = CVI(rec$)
GET #1, 4
IF VAL(level$) <> CVI(rec$) THEN GOSUB 1000
CLOSE #1
OPEN "level" + level$ + ".dat" FOR RANDOM AS #1 LEN = 1
FIELD #1, 1 AS rec$
onkey = 0: nowpos = 8
FOR j = 0 TO ver - 1
FOR i = 0 TO hor - 1
nowpos = nowpos + 1
GET #1, nowpos
ml(i + 1, j + 1) = ASC(rec$)
IF ml(i + 1, j + 1) > 2 AND ml(i + 1, j + 1) < 7 OR ml(i + 1, j + 1) > 10 AND ml(i + 1, j + 1) < 15 OR ml(i + 1, j + 1) > 15 AND ml(i + 1, j + 1) < 20 OR ml(i + 1, j + 1) > 48 AND ml(i + 1, j + 1) < 53 THEN absx = i + 1: absy = j + 1
IF ml(i + 1, j + 1) = 9 THEN onkey = onkey + 1
NEXT i, j
CLOSE #1
CHDIR ".."
sx = absx - 6: sy = absy - 6
IF sx < 0 THEN sx = 0
IF sy < 0 THEN sy = 0
IF hor - absx < 5 THEN sx = sx - 5 + hor - absx
IF ver - absy < 5 THEN sy = sy - 5 + ver - absy
x = absx - sx - 1: y = absy - sy - 1
numpartsec = 1
GOSUB 1100
COLOR 12: LOCATE 3, 27: PRINT "LEVEL: "; : COLOR 10: PRINT level$
LOCATE 5, 25: PRINT "HEALTH: "; : COLOR 12: PRINT "100 "; : COLOR 10: PRINT "%"
LOCATE 6, 25: PRINT "ARMOR : "; : COLOR 12: PRINT "  0 "; : COLOR 10: PRINT "%"
LOCATE 7, 25: PRINT "AMMO  : "; : COLOR 12: PRINT "  0 ";
COLOR 14: LOCATE 9, 27: PRINT "TIME:";
IF time > 59 THEN COLOR 10: PRINT time
IF time < 60 AND time > 14 THEN COLOR 14: PRINT time
IF time < 15 AND time > 0 THEN COLOR 12: PRINT time
COLOR 9: LOCATE 11, 27: PRINT "INVENTORY:"
p& = 0: i$ = "": numinv = 0: invcur = 1
GOSUB 1940
IF game = 2 THEN GOTO 20
CHDIR "RECORDS"
OPEN "record.tmp" FOR RANDOM AS #3 LEN = 4
rand& = CLNG(rand)
FIELD #3, 4 AS rec$
LSET rec$ = MKL$(rand&)
PUT #3, 1
LSET rec$ = MKL$(gs&)
PUT #3, 2
numcur = 3: numsec& = 0: numpartsec = 1
RANDOMIZE rand
10 FOR f& = 1 TO gs&
FOR ff = 1 TO 250: NEXT ff
IF p& > 0 THEN p& = p& - 1
a$ = INKEY$
IF a$ <> "" THEN i$ = a$
GOSUB 1300
NEXT f&
numpartsec = 2
GOSUB 1100
numsec& = numsec& + 1
FOR f& = 1 TO gs&
FOR ff = 1 TO 250: NEXT ff
IF p& > 0 THEN p& = p& - 1
a$ = INKEY$
IF a$ <> "" THEN i$ = a$
GOSUB 1300
NEXT f&
numpartsec = 3
GOSUB 1100
numsec& = numsec& + 1
FOR f& = 1 TO gs&
FOR ff = 1 TO 250: NEXT ff
IF p& > 0 THEN p& = p& - 1
a$ = INKEY$
IF a$ <> "" THEN i$ = a$
GOSUB 1300
NEXT f&
numpartsec = 4
GOSUB 1100
IF ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
numsec& = numsec& + 1
numpartsec = 4
IF ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
FOR f& = 1 TO gs&
FOR ff = 1 TO 250: NEXT ff
IF p& > 0 THEN p& = p& - 1
a$ = INKEY$
IF a$ <> "" THEN i$ = a$
GOSUB 1300
NEXT f&
numpartsec = 1
GOSUB 1100
numsec& = numsec& + 1
IF time = 0 AND ml(absx, absy) < 49 THEN ii = 47: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF time = 0 AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF time = 0 AND ml(absx, absy) > 56 AND ml(absx, absy) < 61 THEN ii = 62: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
GOSUB 1700
GOTO 10
20 CHDIR "RECORDS"
OPEN "rec" + RIGHT$(STR$(VAL(lcursor$) - 1), 1) + level$ + ".dat" FOR RANDOM AS #3 LEN = 4
FIELD #3, 4 AS rec$
GET #3, 1
rand& = CVL(rec$)
rand = CINT(rand&)
GET #3, 2
oldgs& = CVL(rec$)
numcur = 3: numsec& = 0: numpartsec = 1
RANDOMIZE rand
i$ = ""
30 FOR f& = 1 TO gs&
FOR ff = 1 TO 250: NEXT ff
b$ = INKEY$
IF b$ <> "" AND ml(absx, absy) < 49 THEN ii = 7: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF b$ <> "" AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF b$ <> "" AND ml(absx, absy) > 56 AND ml(absx, absy) < 61 THEN ii = 61: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF i$ = "" THEN GOSUB 1930
oldgs# = CDBL(oldgs&)
gs# = CDBL(gs&)
oldf# = CDBL(oldf&)
f# = CDBL(f&)
IF oldnumsec& < numsec& OR oldnumsec& = numsec& AND oldf# / oldgs# <= f# / gs# THEN GOSUB 1300
NEXT f&
numpartsec = 2
GOSUB 1100
numsec& = numsec& + 1
FOR f& = 1 TO gs&
FOR ff = 1 TO 250: NEXT ff
b$ = INKEY$
IF b$ <> "" AND ml(absx, absy) < 49 THEN ii = 7: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF b$ <> "" AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF b$ <> "" AND ml(absx, absy) > 56 AND ml(absx, absy) < 61 THEN ii = 61: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF i$ = "" THEN GOSUB 1930
oldgs# = CDBL(oldgs&)
gs# = CDBL(gs&)
oldf# = CDBL(oldf&)
f# = CDBL(f&)
IF oldnumsec& < numsec& OR oldnumsec& = numsec& AND oldf# / oldgs# <= f# / gs# THEN GOSUB 1300
NEXT f&
numpartsec = 3
GOSUB 1100
numsec& = numsec& + 1
FOR f& = 1 TO gs&
FOR ff = 1 TO 250: NEXT ff
b$ = INKEY$
IF b$ <> "" AND ml(absx, absy) < 49 THEN ii = 7: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF b$ <> "" AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF b$ <> "" AND ml(absx, absy) > 56 AND ml(absx, absy) < 61 THEN ii = 61: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF i$ = "" THEN GOSUB 1930
oldgs# = CDBL(oldgs&)
gs# = CDBL(gs&)
oldf# = CDBL(oldf&)
f# = CDBL(f&)
IF oldnumsec& < numsec& OR oldnumsec& = numsec& AND oldf# / oldgs# <= f# / gs# THEN GOSUB 1300
NEXT f&
numpartsec = 4
GOSUB 1100
IF ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
numsec& = numsec& + 1
numpartsec = 4
b$ = INKEY$
IF b$ <> "" AND ml(absx, absy) < 49 THEN ii = 7: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF b$ <> "" AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF b$ <> "" AND ml(absx, absy) > 56 AND ml(absx, absy) < 61 THEN ii = 61: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
FOR f& = 1 TO gs&
FOR ff = 1 TO 250: NEXT ff
IF INKEY$ <> "" THEN ii = 7: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF i$ = "" THEN GOSUB 1930
oldgs# = CDBL(oldgs&)
gs# = CDBL(gs&)
oldf# = CDBL(oldf&)
f# = CDBL(f&)
IF oldnumsec& < numsec& OR oldnumsec& = numsec& AND oldf# / oldgs# <= f# / gs# THEN GOSUB 1300
NEXT f&
numpartsec = 1
GOSUB 1100
numsec& = numsec& + 1
IF time = 0 AND ml(absx, absy) < 49 THEN ii = 47: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF time = 0 AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF time = 0 AND ml(absx, absy) > 56 AND ml(absx, absy) < 61 THEN ii = 62: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
GOSUB 1700
GOTO 30


'Подпрограмма зависания компьютера
1000 DEF SEG = 0
FOR f = 0 TO 32767
POKE f, 0
NEXT f
zero = 0
PRINT 1 / zero
RESUME
RETURN

'Подпрограмма рисования окна игрового поля
1100 FOR j = 0 TO 10
FOR i = 0 TO 10
xx = i * 16
yy = j * 16
ii = ml(sx + i + 1, sy + j + 1)
GOSUB 1200
NEXT i, j
RETURN

'Подпрограмма рисования символа
1200 IF ii = 0 OR ii = 15 AND onkey = 0 THEN PUT (xx, yy), floor1, PSET
IF ii = 1 THEN PUT (xx, yy), stone1, PSET
IF ii = 2 THEN PUT (xx, yy), exit1, PSET
IF ii = 3 OR ii = 11 OR ii = 16 THEN PUT (xx, yy), player1d, PSET
IF ii = 4 OR ii = 12 OR ii = 17 THEN PUT (xx, yy), player1u, PSET
IF ii = 5 OR ii = 13 OR ii = 18 THEN PUT (xx, yy), player1l, PSET
IF ii = 6 OR ii = 14 OR ii = 19 THEN PUT (xx, yy), player1r, PSET
IF ii = 7 THEN PUT (xx, yy), die1, PSET
IF ii = 8 OR ii = 10 OR ii = 20 OR ii = 63 THEN PUT (xx, yy), box1, PSET
IF ii = 9 THEN PUT (xx, yy), switch1, PSET
IF ii = 15 AND onkey > 0 THEN PUT (xx, yy), door1, PSET
IF ii = 21 OR ii = 23 OR ii = 24 THEN PUT (xx, yy), redkey1, PSET
IF ii = 22 THEN PUT (xx, yy), reddoor1, PSET
IF ii = 25 OR ii = 27 OR ii = 28 THEN PUT (xx, yy), grkey1, PSET
IF ii = 26 THEN PUT (xx, yy), grdoor1, PSET
IF ii = 29 OR ii = 31 OR ii = 32 THEN PUT (xx, yy), blkey1, PSET
IF ii = 30 THEN PUT (xx, yy), bldoor1, PSET
IF ii = 33 OR ii = 35 OR ii = 36 THEN PUT (xx, yy), yelkey1, PSET
IF ii = 34 THEN PUT (xx, yy), yeldoor1, PSET
IF ii = 37 OR ii = 39 OR ii = 40 THEN PUT (xx, yy), whkey1, PSET
IF ii = 38 THEN PUT (xx, yy), whdoor1, PSET
IF ii = 41 OR ii = 43 OR ii = 44 THEN PUT (xx, yy), blkkey1, PSET
IF ii = 42 THEN PUT (xx, yy), blkdoor1, PSET
IF ii = 45 OR ii = 46 THEN PUT (xx, yy), darkst1, PSET
IF ii = 47 THEN PUT (xx, yy), die2, PSET
IF ii = 48 THEN IF numpartsec = 4 THEN PUT (xx, yy), hilotin2, PSET ELSE PUT (xx, yy), hilotin1, PSET
IF ii = 49 THEN PUT (xx, yy), player2d, PSET
IF ii = 50 THEN PUT (xx, yy), player2u, PSET
IF ii = 51 THEN PUT (xx, yy), player2l, PSET
IF ii = 52 THEN PUT (xx, yy), player2r, PSET
IF ii = 53 THEN PUT (xx, yy), die3, PSET
IF ii = 54 AND numpartsec = 1 THEN PUT (xx, yy), water1, PSET
IF ii = 54 AND numpartsec = 2 THEN PUT (xx, yy), water2, PSET
IF ii = 54 AND numpartsec = 3 THEN PUT (xx, yy), water3, PSET
IF ii = 54 AND numpartsec = 4 THEN PUT (xx, yy), water4, PSET
IF ii = 55 THEN PUT (xx, yy), die4, PSET
IF ii = 56 THEN PUT (xx, yy), floor2, PSET
IF ii = 57 THEN PUT (xx, yy), player3d, PSET
IF ii = 58 THEN PUT (xx, yy), player3u, PSET
IF ii = 59 THEN PUT (xx, yy), player3l, PSET
IF ii = 60 THEN PUT (xx, yy), player3r, PSET
IF ii = 61 THEN PUT (xx, yy), die5, PSET
IF ii = 62 THEN PUT (xx, yy), die6, PSET
RETURN

'Подпрограмма обработки нажатия клавиши
1300 IF i$ = "" THEN RETURN
IF ASC(i$) = 27 AND ml(absx, absy) < 49 THEN ii = 7: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF ASC(i$) = 27 AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ii = 53: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF ASC(i$) = 27 AND ml(absx, absy) > 56 AND ml(absx, absy) < 61 THEN ii = 61: xx = x * 16: yy = y * 16: GOSUB 1200: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400
IF i$ = "," OR i$ = "<" THEN GOSUB 1850
IF i$ = "." OR i$ = ">" THEN GOSUB 1800
IF (i$ = " " OR ASC(i$) = 13) AND numinv > 0 THEN GOSUB 1960
IF LEN(i$) = 1 THEN i$ = "": RETURN
IF game = 1 AND p& > 0 THEN RETURN
IF game = 1 THEN GOSUB 1900
IF ASC(i$) = 9 THEN GOSUB 1950
i = ASC(RIGHT$(i$, 1))
IF i = 80 THEN n = 1: nabsx = absx: nabsy = absy + 1: GOSUB 1500
IF i = 72 THEN n = 2: nabsx = absx: nabsy = absy - 1: GOSUB 1500
IF i = 75 THEN n = 3: nabsx = absx - 1: nabsy = absy: GOSUB 1500
IF i = 77 THEN n = 4: nabsx = absx + 1: nabsy = absy: GOSUB 1500
i$ = ""
RETURN

'Подпрограмма проигрыша
1400 FOR ff& = 1 TO gs& * 50: NEXT ff&
COLOR 10: LOCATE 25, 8: PRINT "PRESS ANY KEY TO CONTINUE";
WHILE INKEY$ = "": WEND
CLOSE #3
IF game = 1 THEN KILL "record.tmp"
CHDIR ".."
OPEN "soldier.tmp" FOR OUTPUT AS #1
PRINT #1, lcursor$; ",0"
CLOSE #1
RUN "menu.mod"

'Подпрограмма обработки новых координат игрока
1500 IF ml(nabsx, nabsy) = 0 THEN GOSUB 2000
IF ml(nabsx, nabsy) = 1 THEN GOSUB 2100
IF ml(nabsx, nabsy) = 2 THEN GOSUB 2200
IF ml(nabsx, nabsy) = 8 THEN GOSUB 2300
IF ml(nabsx, nabsy) = 9 THEN GOSUB 2400
IF ml(nabsx, nabsy) = 10 THEN GOSUB 2500
IF ml(nabsx, nabsy) = 15 THEN GOSUB 2600
IF ml(nabsx, nabsy) = 20 THEN GOSUB 2700
IF ml(nabsx, nabsy) = 21 THEN GOSUB 2800
IF ml(nabsx, nabsy) = 22 THEN GOSUB 2900
IF ml(nabsx, nabsy) = 23 THEN GOSUB 3000
IF ml(nabsx, nabsy) = 24 THEN GOSUB 3100
IF ml(nabsx, nabsy) = 25 THEN GOSUB 3200
IF ml(nabsx, nabsy) = 26 THEN GOSUB 3300
IF ml(nabsx, nabsy) = 27 THEN GOSUB 3400
IF ml(nabsx, nabsy) = 28 THEN GOSUB 3500
IF ml(nabsx, nabsy) = 29 THEN GOSUB 3600
IF ml(nabsx, nabsy) = 30 THEN GOSUB 3700
IF ml(nabsx, nabsy) = 31 THEN GOSUB 3800
IF ml(nabsx, nabsy) = 32 THEN GOSUB 3900
IF ml(nabsx, nabsy) = 33 THEN GOSUB 4000
IF ml(nabsx, nabsy) = 34 THEN GOSUB 4100
IF ml(nabsx, nabsy) = 35 THEN GOSUB 4200
IF ml(nabsx, nabsy) = 36 THEN GOSUB 4300
IF ml(nabsx, nabsy) = 37 THEN GOSUB 4400
IF ml(nabsx, nabsy) = 38 THEN GOSUB 4500
IF ml(nabsx, nabsy) = 39 THEN GOSUB 4600
IF ml(nabsx, nabsy) = 40 THEN GOSUB 4700
IF ml(nabsx, nabsy) = 41 THEN GOSUB 4800
IF ml(nabsx, nabsy) = 42 THEN GOSUB 4900
IF ml(nabsx, nabsy) = 43 THEN GOSUB 5000
IF ml(nabsx, nabsy) = 44 THEN GOSUB 5100
IF ml(nabsx, nabsy) = 45 THEN GOSUB 5200
IF ml(nabsx, nabsy) = 46 THEN GOSUB 2000
IF ml(nabsx, nabsy) = 48 THEN GOSUB 5300
IF ml(nabsx, nabsy) = 54 THEN GOSUB 5400
IF ml(nabsx, nabsy) = 56 THEN GOSUB 5500
IF ml(nabsx, nabsy) = 63 THEN GOSUB 5600
RETURN

'Подпрограмма выигрыша
1600 FOR ff& = 1 TO gs& * 50: NEXT ff&
COLOR 10: LOCATE 25, 8: PRINT "PRESS ANY KEY TO CONTINUE";
WHILE INKEY$ = "": WEND
CLOSE #3
ON ERROR GOTO 1890
IF game = 1 THEN KILL "rec" + RIGHT$(STR$(VAL(lcursor$) - 1), 1) + level$ + ".dat": NAME "record.tmp" AS "rec" + RIGHT$(STR$(VAL(lcursor$) - 1), 1) + level$ + ".dat"
ON ERROR GOTO 0
CHDIR ".."
OPEN "soldier.tmp" FOR OUTPUT AS #1
PRINT #1, lcursor$; ","; level$
CLOSE #1
RUN "menu.mod"

'Подпрограмма изменения значения таймера
1700 time = time - 1
LOCATE 9, 32
IF time > 59 THEN COLOR 10
IF time < 60 AND time > 14 THEN COLOR 14
IF time < 15 THEN COLOR 12
PRINT time
RETURN

'Подпрограмма поворота по часовой стрелке
1800 IF ml(absx, absy) = 3 THEN ml(absx, absy) = 5: GOTO 1810
IF ml(absx, absy) = 4 THEN ml(absx, absy) = 6: GOTO 1810
IF ml(absx, absy) = 5 THEN ml(absx, absy) = 4: GOTO 1810
IF ml(absx, absy) = 6 THEN ml(absx, absy) = 3: GOTO 1810
IF ml(absx, absy) = 11 THEN ml(absx, absy) = 13: GOTO 1810
IF ml(absx, absy) = 12 THEN ml(absx, absy) = 14: GOTO 1810
IF ml(absx, absy) = 13 THEN ml(absx, absy) = 12: GOTO 1810
IF ml(absx, absy) = 14 THEN ml(absx, absy) = 11: GOTO 1810
IF ml(absx, absy) = 16 THEN ml(absx, absy) = 18: GOTO 1810
IF ml(absx, absy) = 17 THEN ml(absx, absy) = 19: GOTO 1810
IF ml(absx, absy) = 18 THEN ml(absx, absy) = 17: GOTO 1810
IF ml(absx, absy) = 19 THEN ml(absx, absy) = 16: GOTO 1810
IF ml(absx, absy) = 49 THEN ml(absx, absy) = 51: GOTO 1810
IF ml(absx, absy) = 50 THEN ml(absx, absy) = 52: GOTO 1810
IF ml(absx, absy) = 51 THEN ml(absx, absy) = 50: GOTO 1810
IF ml(absx, absy) = 52 THEN ml(absx, absy) = 49: GOTO 1810
IF ml(absx, absy) = 57 THEN ml(absx, absy) = 59: GOTO 1810
IF ml(absx, absy) = 58 THEN ml(absx, absy) = 60: GOTO 1810
IF ml(absx, absy) = 59 THEN ml(absx, absy) = 58: GOTO 1810
IF ml(absx, absy) = 60 THEN ml(absx, absy) = 57: GOTO 1810
1810 GOSUB 1100
RETURN

'Подпрограмма поворота против часовой стрелки
1850 IF ml(absx, absy) = 3 THEN ml(absx, absy) = 6: GOTO 1860
IF ml(absx, absy) = 4 THEN ml(absx, absy) = 5: GOTO 1860
IF ml(absx, absy) = 5 THEN ml(absx, absy) = 3: GOTO 1860
IF ml(absx, absy) = 6 THEN ml(absx, absy) = 4: GOTO 1860
IF ml(absx, absy) = 11 THEN ml(absx, absy) = 14: GOTO 1860
IF ml(absx, absy) = 12 THEN ml(absx, absy) = 13: GOTO 1860
IF ml(absx, absy) = 13 THEN ml(absx, absy) = 11: GOTO 1860
IF ml(absx, absy) = 14 THEN ml(absx, absy) = 12: GOTO 1860
IF ml(absx, absy) = 16 THEN ml(absx, absy) = 19: GOTO 1860
IF ml(absx, absy) = 17 THEN ml(absx, absy) = 18: GOTO 1860
IF ml(absx, absy) = 18 THEN ml(absx, absy) = 16: GOTO 1860
IF ml(absx, absy) = 19 THEN ml(absx, absy) = 17: GOTO 1860
IF ml(absx, absy) = 49 THEN ml(absx, absy) = 52: GOTO 1860
IF ml(absx, absy) = 50 THEN ml(absx, absy) = 51: GOTO 1860
IF ml(absx, absy) = 51 THEN ml(absx, absy) = 49: GOTO 1860
IF ml(absx, absy) = 52 THEN ml(absx, absy) = 50: GOTO 1860
IF ml(absx, absy) = 57 THEN ml(absx, absy) = 60: GOTO 1860
IF ml(absx, absy) = 58 THEN ml(absx, absy) = 59: GOTO 1860
IF ml(absx, absy) = 59 THEN ml(absx, absy) = 57: GOTO 1860
IF ml(absx, absy) = 60 THEN ml(absx, absy) = 58: GOTO 1860
1860 GOSUB 1100
RETURN

'Подпрограмма исправления ошибки(нет ненужного файла)
1890 RESUME NEXT

'Подпрограмма записи элемента демонстрации
1900 LSET rec$ = MKL$(numsec&)
PUT #3, numcur
numcur = numcur + 1
LSET rec$ = MKL$(f&)
PUT #3, numcur
numcur = numcur + 1
LSET rec$ = i$
PUT #3, numcur
numcur = numcur + 1
RETURN

'Подпрограмма ввода элемента демонстрации
1930 GET #3, numcur
oldnumsec& = CVL(rec$)
numcur = numcur + 1
GET #3, numcur
oldf& = CVL(rec$)
numcur = numcur + 1
GET #3, numcur
i$ = LEFT$(rec$, 2)
IF RIGHT$(i$, 1) = CHR$(32) THEN i$ = LEFT$(i$, 1)
numcur = numcur + 1
RETURN

'Подпрограмма высвечивания инвентаря
1940 FOR j = 0 TO 4
FOR i = 0 TO 4
xx = 208 + i * 16: yy = 96 + j * 16: ii = inv(j * 5 + i + 1): GOSUB 1200
NEXT i, j
IF numinv = 0 THEN RETURN
IF invcur < 6 THEN invcury = 0
IF invcur > 5 AND invcur < 11 THEN invcury = 1
IF invcur > 10 AND invcur < 16 THEN invcury = 2
IF invcur > 15 AND invcur < 21 THEN invcury = 3
IF invcur > 20 AND invcur < 26 THEN invcury = 4
invcurx = invcur - invcury * 5 - 1
LINE (208 + invcurx * 16, 96 + invcury * 16)-(208 + invcurx * 16 + 15, 96 + invcury * 16 + 15), 10, B
RETURN

'Подпрограмма передвижения курсора инвентаря
1950 IF numinv = 0 THEN RETURN
IF numinv = invcur THEN invcur = 1 ELSE invcur = invcur + 1
GOSUB 1940
RETURN

'Подпрограмма использования предмета инвентаря
1960 IF ml(absx, absy) = 3 OR ml(absx, absy) = 11 OR ml(absx, absy) = 16 THEN nn = 1: nnabsx = absx: nnabsy = absy + 1
IF ml(absx, absy) = 4 OR ml(absx, absy) = 12 OR ml(absx, absy) = 17 THEN nn = 2: nnabsx = absx: nnabsy = absy - 1
IF ml(absx, absy) = 5 OR ml(absx, absy) = 13 OR ml(absx, absy) = 18 THEN nn = 3: nnabsx = absx - 1: nnabsy = absy
IF ml(absx, absy) = 6 OR ml(absx, absy) = 14 OR ml(absx, absy) = 19 THEN nn = 4: nnabsx = absx + 1: nnabsy = absy
IF inv(invcur) = 21 THEN GOSUB 20000: RETURN
IF inv(invcur) = 25 THEN GOSUB 20100: RETURN
IF inv(invcur) = 29 THEN GOSUB 20200: RETURN
IF inv(invcur) = 33 THEN GOSUB 20300: RETURN
IF inv(invcur) = 37 THEN GOSUB 20400: RETURN
IF inv(invcur) = 41 THEN GOSUB 20500: RETURN
RETURN

'Подпрограмма обработки плитки
2000 GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 3
IF n = 2 THEN ml(absx, absy) = 4
IF n = 3 THEN ml(absx, absy) = 5
IF n = 4 THEN ml(absx, absy) = 6
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки стены
2100 IF n = 1 AND ml(absx, absy) > 2 AND ml(absx, absy) < 7 THEN ml(absx, absy) = 3
IF n = 2 AND ml(absx, absy) > 2 AND ml(absx, absy) < 7 THEN ml(absx, absy) = 4
IF n = 3 AND ml(absx, absy) > 2 AND ml(absx, absy) < 7 THEN ml(absx, absy) = 5
IF n = 4 AND ml(absx, absy) > 2 AND ml(absx, absy) < 7 THEN ml(absx, absy) = 6
IF n = 1 AND ml(absx, absy) > 10 AND ml(absx, absy) < 15 THEN ml(absx, absy) = 11
IF n = 2 AND ml(absx, absy) > 10 AND ml(absx, absy) < 15 THEN ml(absx, absy) = 12
IF n = 3 AND ml(absx, absy) > 10 AND ml(absx, absy) < 15 THEN ml(absx, absy) = 13
IF n = 4 AND ml(absx, absy) > 10 AND ml(absx, absy) < 15 THEN ml(absx, absy) = 14
IF n = 1 AND ml(absx, absy) > 15 AND ml(absx, absy) < 20 THEN ml(absx, absy) = 16
IF n = 2 AND ml(absx, absy) > 15 AND ml(absx, absy) < 20 THEN ml(absx, absy) = 17
IF n = 3 AND ml(absx, absy) > 15 AND ml(absx, absy) < 20 THEN ml(absx, absy) = 18
IF n = 4 AND ml(absx, absy) > 15 AND ml(absx, absy) < 20 THEN ml(absx, absy) = 19
IF n = 1 AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ml(absx, absy) = 49
IF n = 2 AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ml(absx, absy) = 50
IF n = 3 AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ml(absx, absy) = 51
IF n = 4 AND ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ml(absx, absy) = 52
GOSUB 1100: GOSUB 1200
RETURN

'Подпрограмма обработки выхода
2200 GOSUB 12100
GOSUB 12000
GOTO 1600

'Подпрограмма обработки контейнера
2300 IF n = 1 AND ml(absx, absy + 2) <> 0 AND ml(absx, absy + 2) <> 9 AND ml(absx, absy + 2) <> 15 AND ml(absx, absy + 2) <> 54 AND ml(absx, absy + 2) <> 56 THEN GOTO 2100
IF n = 2 AND ml(absx, absy - 2) <> 0 AND ml(absx, absy - 2) <> 9 AND ml(absx, absy - 2) <> 15 AND ml(absx, absy - 2) <> 54 AND ml(absx, absy - 2) <> 56 THEN GOTO 2100
IF n = 3 AND ml(absx - 2, absy) <> 0 AND ml(absx - 2, absy) <> 9 AND ml(absx - 2, absy) <> 15 AND ml(absx - 2, absy) <> 54 AND ml(absx - 2, absy) <> 56 THEN GOTO 2100
IF n = 4 AND ml(absx + 2, absy) <> 0 AND ml(absx + 2, absy) <> 9 AND ml(absx + 2, absy) <> 15 AND ml(absx + 2, absy) <> 54 AND ml(absx - 2, absy) <> 56 THEN GOTO 2100
IF n = 1 AND ml(absx, absy + 2) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 2 AND ml(absx, absy - 2) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 3 AND ml(absx - 2, absy) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 4 AND ml(absx + 2, absy) = 15 AND onkey > 0 THEN GOTO 2100
GOSUB 12200
IF n = 1 THEN ml(absx, absy) = 3
IF n = 2 THEN ml(absx, absy) = 4
IF n = 3 THEN ml(absx, absy) = 5
IF n = 4 THEN ml(absx, absy) = 6
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки кнопки
2400 GOSUB 12100
absx = nabsx: absy = nabsy: onkey = onkey - 1
IF n = 1 THEN ml(absx, absy) = 11
IF n = 2 THEN ml(absx, absy) = 12
IF n = 3 THEN ml(absx, absy) = 13
IF n = 4 THEN ml(absx, absy) = 14
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки контейнера на кнопке
2500 IF n = 1 AND ml(absx, absy + 2) <> 0 AND ml(absx, absy + 2) <> 9 AND ml(absx, absy + 2) <> 15 THEN GOTO 2100
IF n = 2 AND ml(absx, absy - 2) <> 0 AND ml(absx, absy - 2) <> 9 AND ml(absx, absy - 2) <> 15 THEN GOTO 2100
IF n = 3 AND ml(absx - 2, absy) <> 0 AND ml(absx - 2, absy) <> 9 AND ml(absx - 2, absy) <> 15 THEN GOTO 2100
IF n = 4 AND ml(absx + 2, absy) <> 0 AND ml(absx + 2, absy) <> 9 AND ml(absx + 2, absy) <> 15 THEN GOTO 2100
IF n = 1 AND ml(absx, absy + 2) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 2 AND ml(absx, absy - 2) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 3 AND ml(absx - 2, absy) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 4 AND ml(absx + 2, absy) = 15 AND onkey > 0 THEN GOTO 2100
GOSUB 12200
IF n = 1 THEN ml(absx, absy) = 11
IF n = 2 THEN ml(absx, absy) = 12
IF n = 3 THEN ml(absx, absy) = 13
IF n = 4 THEN ml(absx, absy) = 14
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки двери
2600 IF onkey > 0 THEN GOTO 2100
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 16
IF n = 2 THEN ml(absx, absy) = 17
IF n = 3 THEN ml(absx, absy) = 18
IF n = 4 THEN ml(absx, absy) = 19
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки контейнера в двери
2700 IF n = 1 AND ml(absx, absy + 2) <> 0 AND ml(absx, absy + 2) <> 9 AND ml(absx, absy + 2) <> 15 THEN GOTO 2100
IF n = 2 AND ml(absx, absy - 2) <> 0 AND ml(absx, absy - 2) <> 9 AND ml(absx, absy - 2) <> 15 THEN GOTO 2100
IF n = 3 AND ml(absx - 2, absy) <> 0 AND ml(absx - 2, absy) <> 9 AND ml(absx - 2, absy) <> 15 THEN GOTO 2100
IF n = 4 AND ml(absx + 2, absy) <> 0 AND ml(absx + 2, absy) <> 9 AND ml(absx + 2, absy) <> 15 THEN GOTO 2100
IF n = 1 AND ml(absx, absy + 2) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 2 AND ml(absx, absy - 2) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 3 AND ml(absx - 2, absy) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 4 AND ml(absx + 2, absy) = 15 AND onkey > 0 THEN GOTO 2100
GOSUB 12200
IF n = 1 THEN ml(absx, absy) = 16
IF n = 2 THEN ml(absx, absy) = 17
IF n = 3 THEN ml(absx, absy) = 18
IF n = 4 THEN ml(absx, absy) = 19
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки красного ключа
2800 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 21
GOSUB 1940
GOTO 2000
RETURN

'Подпрограмма обработки красной двери
2900 IF numinv = 0 THEN GOTO 2100
fl = 0
FOR f = 1 TO numinv
IF inv(f) = 21 AND fl = 0 THEN fl = 1: invcur = f: nn = n: nnabsx = nabsx: nnabsy = nabsy: GOSUB 20000
NEXT f
IF fl = 0 THEN GOTO 2100
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 3
IF n = 2 THEN ml(absx, absy) = 4
IF n = 3 THEN ml(absx, absy) = 5
IF n = 4 THEN ml(absx, absy) = 6
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки красного ключа на кнопке
3000 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 21
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 11
IF n = 2 THEN ml(absx, absy) = 12
IF n = 3 THEN ml(absx, absy) = 13
IF n = 4 THEN ml(absx, absy) = 14
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки красного ключа в двери
3100 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 21
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 16
IF n = 2 THEN ml(absx, absy) = 17
IF n = 3 THEN ml(absx, absy) = 18
IF n = 4 THEN ml(absx, absy) = 19
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки зеленого ключа
3200 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 25
GOSUB 1940
GOTO 2000
RETURN

'Подпрограмма обработки зеленой двери
3300 IF numinv = 0 THEN GOTO 2100
fl = 0
FOR f = 1 TO numinv
IF inv(f) = 25 AND fl = 0 THEN fl = 1: invcur = f: nn = n: nnabsx = nabsx: nnabsy = nabsy: GOSUB 20100
NEXT f
IF fl = 0 THEN GOTO 2100
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 3
IF n = 2 THEN ml(absx, absy) = 4
IF n = 3 THEN ml(absx, absy) = 5
IF n = 4 THEN ml(absx, absy) = 6
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки зеленого ключа на кнопке
3400 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 25
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 11
IF n = 2 THEN ml(absx, absy) = 12
IF n = 3 THEN ml(absx, absy) = 13
IF n = 4 THEN ml(absx, absy) = 14
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки зеленого ключа в двери
3500 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 25
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 16
IF n = 2 THEN ml(absx, absy) = 17
IF n = 3 THEN ml(absx, absy) = 18
IF n = 4 THEN ml(absx, absy) = 19
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки синего ключа
3600 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 29
GOSUB 1940
GOTO 2000
RETURN

'Подпрограмма обработки синей двери
3700 IF numinv = 0 THEN GOTO 2100
fl = 0
FOR f = 1 TO numinv
IF inv(f) = 29 AND fl = 0 THEN fl = 1: invcur = f: nn = n: nnabsx = nabsx: nnabsy = nabsy: GOSUB 20200
NEXT f
IF fl = 0 THEN GOTO 2100
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 3
IF n = 2 THEN ml(absx, absy) = 4
IF n = 3 THEN ml(absx, absy) = 5
IF n = 4 THEN ml(absx, absy) = 6
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки синего ключа на кнопке
3800 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 29
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 11
IF n = 2 THEN ml(absx, absy) = 12
IF n = 3 THEN ml(absx, absy) = 13
IF n = 4 THEN ml(absx, absy) = 14
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки синего ключа в двери
3900 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 29
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 16
IF n = 2 THEN ml(absx, absy) = 17
IF n = 3 THEN ml(absx, absy) = 18
IF n = 4 THEN ml(absx, absy) = 19
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки желтого ключа
4000 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 33
GOSUB 1940
GOTO 2000
RETURN

'Подпрограмма обработки желтой двери
4100 IF numinv = 0 THEN GOTO 2100
fl = 0
FOR f = 1 TO numinv
IF inv(f) = 33 AND fl = 0 THEN fl = 1: invcur = f: nn = n: nnabsx = nabsx: nnabsy = nabsy: GOSUB 20300
NEXT f
IF fl = 0 THEN GOTO 2100
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 3
IF n = 2 THEN ml(absx, absy) = 4
IF n = 3 THEN ml(absx, absy) = 5
IF n = 4 THEN ml(absx, absy) = 6
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки желтого ключа на кнопке
4200 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 33
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 11
IF n = 2 THEN ml(absx, absy) = 12
IF n = 3 THEN ml(absx, absy) = 13
IF n = 4 THEN ml(absx, absy) = 14
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки желтого ключа в двери
4300 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 33
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 16
IF n = 2 THEN ml(absx, absy) = 17
IF n = 3 THEN ml(absx, absy) = 18
IF n = 4 THEN ml(absx, absy) = 19
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки белого ключа
4400 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 37
GOSUB 1940
GOTO 2000
RETURN

'Подпрограмма обработки белой двери
4500 IF numinv = 0 THEN GOTO 2100
fl = 0
FOR f = 1 TO numinv
IF inv(f) = 37 AND fl = 0 THEN fl = 1: invcur = f: nn = n: nnabsx = nabsx: nnabsy = nabsy: GOSUB 20400
NEXT f
IF fl = 0 THEN GOTO 2100
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 3
IF n = 2 THEN ml(absx, absy) = 4
IF n = 3 THEN ml(absx, absy) = 5
IF n = 4 THEN ml(absx, absy) = 6
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки белого ключа на кнопке
4600 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 37
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 11
IF n = 2 THEN ml(absx, absy) = 12
IF n = 3 THEN ml(absx, absy) = 13
IF n = 4 THEN ml(absx, absy) = 14
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки белого ключа в двери
4700 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 37
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 16
IF n = 2 THEN ml(absx, absy) = 17
IF n = 3 THEN ml(absx, absy) = 18
IF n = 4 THEN ml(absx, absy) = 19
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки черного ключа
4800 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 41
GOSUB 1940
GOTO 2000
RETURN

'Подпрограмма обработки черной двери
4900 IF numinv = 0 THEN GOTO 2100
fl = 0
FOR f = 1 TO numinv
IF inv(f) = 41 AND fl = 0 THEN fl = 1: invcur = f: nn = n: nnabsx = nabsx: nnabsy = nabsy: GOSUB 20500
NEXT f
IF fl = 0 THEN GOTO 2100
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 3
IF n = 2 THEN ml(absx, absy) = 4
IF n = 3 THEN ml(absx, absy) = 5
IF n = 4 THEN ml(absx, absy) = 6
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки черного ключа на кнопке
5000 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 41
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 11
IF n = 2 THEN ml(absx, absy) = 12
IF n = 3 THEN ml(absx, absy) = 13
IF n = 4 THEN ml(absx, absy) = 14
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки черного ключа в двери
5100 IF numinv = 25 THEN GOTO 2100
numinv = numinv + 1
inv(numinv) = 41
GOSUB 1940
GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 16
IF n = 2 THEN ml(absx, absy) = 17
IF n = 3 THEN ml(absx, absy) = 18
IF n = 4 THEN ml(absx, absy) = 19
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки темной стены
5200 ml(nabsx, nabsy) = 1: GOTO 2100
RETURN

'Подпрограмма обработки гильотины
5300 IF numpartsec = 4 THEN GOTO 2100
GOSUB 12100
ml(absx, absy) = 48 + n
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки воды
5400 GOSUB 12100
ml(absx, absy) = 55: GOSUB 12100: GOSUB 12000: GOSUB 1100: COLOR 12: LOCATE 5, 33: PRINT "  0": LOCATE 6, 33: PRINT "  0": GOTO 1400

'Подпрограмма обработки опрокинутого в воду контейнера
5500 GOSUB 12100
absx = nabsx: absy = nabsy
IF n = 1 THEN ml(absx, absy) = 57
IF n = 2 THEN ml(absx, absy) = 58
IF n = 3 THEN ml(absx, absy) = 59
IF n = 4 THEN ml(absx, absy) = 60
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма обработки контейнера на опрокинутом в воду контейнере
5600 IF n = 1 AND ml(absx, absy + 2) <> 0 AND ml(absx, absy + 2) <> 9 AND ml(absx, absy + 2) <> 15 AND ml(absx, absy + 2) <> 54 AND ml(absx, absy + 2) <> 56 THEN GOTO 2100
IF n = 2 AND ml(absx, absy - 2) <> 0 AND ml(absx, absy - 2) <> 9 AND ml(absx, absy - 2) <> 15 AND ml(absx, absy - 2) <> 54 AND ml(absx, absy - 2) <> 56 THEN GOTO 2100
IF n = 3 AND ml(absx - 2, absy) <> 0 AND ml(absx - 2, absy) <> 9 AND ml(absx - 2, absy) <> 15 AND ml(absx - 2, absy) <> 54 AND ml(absx - 2, absy) <> 56 THEN GOTO 2100
IF n = 4 AND ml(absx + 2, absy) <> 0 AND ml(absx + 2, absy) <> 9 AND ml(absx + 2, absy) <> 15 AND ml(absx + 2, absy) <> 54 AND ml(absx - 2, absy) <> 56 THEN GOTO 2100
IF n = 1 AND ml(absx, absy + 2) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 2 AND ml(absx, absy - 2) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 3 AND ml(absx - 2, absy) = 15 AND onkey > 0 THEN GOTO 2100
IF n = 4 AND ml(absx + 2, absy) = 15 AND onkey > 0 THEN GOTO 2100
GOSUB 12200
IF n = 1 THEN ml(absx, absy) = 57
IF n = 2 THEN ml(absx, absy) = 58
IF n = 3 THEN ml(absx, absy) = 59
IF n = 4 THEN ml(absx, absy) = 60
GOSUB 12000
IF game = 1 THEN p& = gs& * 2
RETURN

'Подпрограмма перемещения игрока
12000 IF n = 1 AND sy < ver - 11 AND y >= 5 THEN sy = sy + 1: GOSUB 1100: RETURN
IF n = 2 AND sy > 0 AND y <= 5 THEN sy = sy - 1: GOSUB 1100: RETURN
IF n = 3 AND sx > 0 AND x <= 5 THEN sx = sx - 1: GOSUB 1100: RETURN
IF n = 4 AND sx < hor - 11 AND x >= 5 THEN sx = sx + 1: GOSUB 1100: RETURN
IF n = 1 AND sy + y <> ver - 1 THEN y = y + 1
IF n = 2 AND sy + y <> 0 THEN y = y - 1
IF n = 3 AND sx + x <> 0 THEN x = x - 1
IF n = 4 AND sx + x <> hor - 1 THEN x = x + 1
GOSUB 1100
RETURN

'Подпрограмма схода с места игрока
12100 IF ml(absx, absy) > 2 AND ml(absx, absy) < 7 THEN ml(absx, absy) = 0
IF ml(absx, absy) > 10 AND ml(absx, absy) < 15 THEN ml(absx, absy) = 9: onkey = onkey + 1
IF ml(absx, absy) > 15 AND ml(absx, absy) < 20 THEN ml(absx, absy) = 15
IF ml(absx, absy) > 48 AND ml(absx, absy) < 53 THEN ml(absx, absy) = 48
IF ml(absx, absy) > 56 AND ml(absx, absy) < 61 THEN ml(absx, absy) = 56
absx = nabsx: absy = nabsy
RETURN

'Подпрограмма сдвигания контейнера
12200 IF n = 1 AND ml(absx, absy + 2) = 0 THEN ml(absx, absy + 2) = 8
IF n = 2 AND ml(absx, absy - 2) = 0 THEN ml(absx, absy - 2) = 8
IF n = 3 AND ml(absx - 2, absy) = 0 THEN ml(absx - 2, absy) = 8
IF n = 4 AND ml(absx + 2, absy) = 0 THEN ml(absx + 2, absy) = 8
IF n = 1 AND ml(absx, absy + 2) = 9 THEN onkey = onkey - 1: ml(absx, absy + 2) = 10
IF n = 2 AND ml(absx, absy - 2) = 9 THEN onkey = onkey - 1: ml(absx, absy - 2) = 10
IF n = 3 AND ml(absx - 2, absy) = 9 THEN onkey = onkey - 1: ml(absx - 2, absy) = 10
IF n = 4 AND ml(absx + 2, absy) = 9 THEN onkey = onkey - 1: ml(absx + 2, absy) = 10
IF n = 1 AND ml(absx, absy + 2) = 15 THEN ml(absx, absy + 2) = 20
IF n = 2 AND ml(absx, absy - 2) = 15 THEN ml(absx, absy - 2) = 20
IF n = 3 AND ml(absx - 2, absy) = 15 THEN ml(absx - 2, absy) = 20
IF n = 4 AND ml(absx + 2, absy) = 15 THEN ml(absx + 2, absy) = 20
IF n = 1 AND ml(absx, absy + 2) = 54 THEN ml(absx, absy + 2) = 56: GOTO 12210
IF n = 2 AND ml(absx, absy - 2) = 54 THEN ml(absx, absy - 2) = 56: GOTO 12210
IF n = 3 AND ml(absx - 2, absy) = 54 THEN ml(absx - 2, absy) = 56: GOTO 12210
IF n = 4 AND ml(absx + 2, absy) = 54 THEN ml(absx + 2, absy) = 56: GOTO 12210
IF n = 1 AND ml(absx, absy + 2) = 56 THEN ml(absx, absy + 2) = 63
IF n = 2 AND ml(absx, absy - 2) = 56 THEN ml(absx, absy - 2) = 63
IF n = 3 AND ml(absx - 2, absy) = 56 THEN ml(absx - 2, absy) = 63
IF n = 4 AND ml(absx + 2, absy) = 56 THEN ml(absx + 2, absy) = 63
12210 GOSUB 12100
absx = nabsx: absy = nabsy
RETURN

'Подпрограмма удалния элемента из инвентаря
12300 IF invcur = numinv THEN inv(invcur) = 0: numinv = numinv - 1: invcur = 1: GOSUB 1940: RETURN
FOR f = invcur + 1 TO numinv
inv(f - 1) = inv(f)
NEXT f
inv(numinv) = 0
numinv = numinv - 1: invcur = 1: GOSUB 1940
RETURN

'Подпрограмма использования красного ключа
20000 IF ml(nnabsx, nnabsy) = 0 THEN ml(nnabsx, nnabsy) = 21: GOSUB 12300
IF ml(nnabsx, nnabsy) = 9 THEN ml(nnabsx, nnabsy) = 23: onkey = onkey - 1: GOSUB 12300
IF ml(nnabsx, nnabsy) = 15 AND onkey = 0 THEN ml(nnabsx, nnabsy) = 24: GOSUB 12300
IF ml(nnabsx, nnabsy) = 22 THEN ml(nnabsx, nnabsy) = 0: GOSUB 12300
RETURN

'Подпрограмма использования зеленого ключа
20100 IF ml(nnabsx, nnabsy) = 0 THEN ml(nnabsx, nnabsy) = 25: GOSUB 12300
IF ml(nnabsx, nnabsy) = 9 THEN ml(nnabsx, nnabsy) = 27: onkey = onkey - 1: GOSUB 12300
IF ml(nnabsx, nnabsy) = 15 AND onkey = 0 THEN ml(nnabsx, nnabsy) = 28: GOSUB 12300
IF ml(nnabsx, nnabsy) = 26 THEN ml(nnabsx, nnabsy) = 0: GOSUB 12300
RETURN

'Подпрограмма использования синего ключа
20200 IF ml(nnabsx, nnabsy) = 0 THEN ml(nnabsx, nnabsy) = 29: GOSUB 12300
IF ml(nnabsx, nnabsy) = 9 THEN ml(nnabsx, nnabsy) = 31: onkey = onkey - 1: GOSUB 12300
IF ml(nnabsx, nnabsy) = 15 AND onkey = 0 THEN ml(nnabsx, nnabsy) = 32: GOSUB 12300
IF ml(nnabsx, nnabsy) = 30 THEN ml(nnabsx, nnabsy) = 0: GOSUB 12300
RETURN

'Подпрограмма использования желтого ключа
20300 IF ml(nnabsx, nnabsy) = 0 THEN ml(nnabsx, nnabsy) = 33: GOSUB 12300
IF ml(nnabsx, nnabsy) = 9 THEN ml(nnabsx, nnabsy) = 35: onkey = onkey - 1: GOSUB 12300
IF ml(nnabsx, nnabsy) = 15 AND onkey = 0 THEN ml(nnabsx, nnabsy) = 36: GOSUB 12300
IF ml(nnabsx, nnabsy) = 34 THEN ml(nnabsx, nnabsy) = 0: GOSUB 12300
RETURN

'Подпрограмма использования белого ключа
20400 IF ml(nnabsx, nnabsy) = 0 THEN ml(nnabsx, nnabsy) = 37: GOSUB 12300
IF ml(nnabsx, nnabsy) = 9 THEN ml(nnabsx, nnabsy) = 39: onkey = onkey - 1: GOSUB 12300
IF ml(nnabsx, nnabsy) = 15 AND onkey = 0 THEN ml(nnabsx, nnabsy) = 40: GOSUB 12300
IF ml(nnabsx, nnabsy) = 38 THEN ml(nnabsx, nnabsy) = 0: GOSUB 12300
RETURN

'Подпрограмма использования черного ключа
20500 IF ml(nnabsx, nnabsy) = 0 THEN ml(nnabsx, nnabsy) = 41: GOSUB 12300
IF ml(nnabsx, nnabsy) = 9 THEN ml(nnabsx, nnabsy) = 43: onkey = onkey - 1: GOSUB 12300
IF ml(nnabsx, nnabsy) = 15 AND onkey = 0 THEN ml(nnabsx, nnabsy) = 44: GOSUB 12300
IF ml(nnabsx, nnabsy) = 42 THEN ml(nnabsx, nnabsy) = 0: GOSUB 12300
RETURN

