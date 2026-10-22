DEFINT A-Z
DIM menu$(5), saves$(10), curlev(10)
RANDOMIZE VAL(LEFT$(TIME$, 2)) * 10000 + VAL(MID$(TIME$, 4, 2)) * 100 + VAL(RIGHT$(TIME$, 2))
ON ERROR GOTO 1000
OPEN "soldier.tmp" FOR INPUT AS #3
ON ERROR GOTO 0
FOR f = 0 TO 9
ON ERROR GOTO 1100
OPEN "gamesav" + CHR$(48 + f) + ".dat" FOR INPUT AS #1
CLOSE #1
ON ERROR GOTO 0
OPEN "gamesav" + CHR$(48 + f) + ".dat" FOR RANDOM AS #1 LEN = 2
FIELD #1, 2 AS rec$
saves$(f + 1) = ""
FOR ff = 1 TO 4
GET #1, ff
saves$(f + 1) = saves$(f + 1) + rec$
NEXT ff
CLOSE #1
OPEN "gamesav" + CHR$(48 + f) + ".dat" FOR RANDOM AS #1 LEN = 2
FIELD #1, 2 AS rec$
OPEN "gamesav" + CHR$(48 + f) + ".dat" FOR RANDOM AS #2 LEN = 2
FIELD #2, 2 AS rec2$
FOR ff = 5 TO 106
GET #1, ff
GET #2, 213 - ff
IF rec$ <> MKI$(CVI(rec2$) - 1) THEN CLOSE #1, #2, #3: KILL "soldier.tmp": GOSUB 1000
NEXT ff
CLOSE #2
GET #1, 66
curlev(f + 1) = CVI(rec$)
CLOSE #1
10 ON ERROR GOTO 0
NEXT f
lcursor = 1
INPUT #3, a$
lcursor = VAL(a$)
INPUT #3, level$
IF level$ <> "0" THEN GOSUB 1200
CLOSE #3
KILL "soldier.tmp"
OPEN "soldier.cfg" FOR INPUT AS #1
INPUT #1, gs&
INPUT #1, ls&
CLOSE #1
menu$(1) = "NEW GAME     "
menu$(2) = "REPLAY LEVEL "
menu$(3) = "DELETE PLAYER"
menu$(4) = "GAME SPEED   "
menu$(5) = "REPLAY SPEED "
rcursor = 1
SCREEN 0: WIDTH 40: CLS : COLOR 10
LOCATE 4, 17, 0: PRINT "SOLDIER"
COLOR 14
FOR f = 1 TO 5
IF f = 1 THEN COLOR , 1 ELSE COLOR , 0
LOCATE 11 + f, 24: PRINT menu$(f)
NEXT f
COLOR 12: LOCATE 8, 4: PRINT "PLAYERS  LEVEL"
FOR f = 1 TO 10
IF f = lcursor THEN COLOR 15 ELSE COLOR 3
LOCATE 9 + f, 3: PRINT saves$(f); "  "; curlev(f)
NEXT f
20 i$ = INKEY$
IF i$ = "" THEN GOTO 20
IF ASC(i$) = 9 THEN LOCATE 9 + lcursor, 3: COLOR 3: PRINT saves$(lcursor); "  "; curlev(lcursor): lcursor = lcursor + 1: IF lcursor = 11 THEN lcursor = 1
IF ASC(i$) = 9 THEN LOCATE 9 + lcursor, 3: COLOR 15: PRINT saves$(lcursor); "  "; curlev(lcursor)
IF ASC(i$) = 27 THEN COLOR 7: CLS : SCREEN 0: WIDTH 80: END
IF ASC(i$) = 13 OR i$ = " " THEN ON rcursor GOTO 1300, 1800, 1400, 1500, 1900
IF LEN(i$) = 1 THEN GOTO 20
i = ASC(RIGHT$(i$, 1))
IF i = 80 THEN LOCATE 11 + rcursor, 24: COLOR 14: PRINT menu$(rcursor): rcursor = rcursor + 1: IF rcursor = 6 THEN rcursor = 1
IF i = 72 THEN LOCATE 11 + rcursor, 24: COLOR 14: PRINT menu$(rcursor): rcursor = rcursor - 1: IF rcursor = 0 THEN rcursor = 5
IF i = 80 OR i = 72 THEN LOCATE 11 + rcursor, 24: COLOR 14, 1: PRINT menu$(rcursor): COLOR , 0
GOTO 20


'Подпрограмма зависания компьютера
1000 DEF SEG = 0
FOR f = 0 TO 32767
POKE f, 0
NEXT f
zero = 0
PRINT 1 / zero
RESUME
RETURN

'Подпрограмма исправления ошибки(нет файла подписки)
1100 saves$(f + 1) = "--------"
curlev(f + 1) = 0
RESUME 10

'Подпрограмма увеличения счетчика уровней
1200 IF VAL(level$) < curlev(lcursor) THEN RETURN
curlev(lcursor) = VAL(level$) + 1
OPEN "gamesav" + RIGHT$(STR$(lcursor - 1), 1) + ".dat" FOR RANDOM AS #1 LEN = 2
FIELD #1, 2 AS rec$
OPEN "gamesav" + RIGHT$(STR$(lcursor - 1), 1) + ".dat" FOR RANDOM AS #2 LEN = 2
FIELD #2, 2 AS rec2$
FOR f = 5 TO 106
a$ = MKI$(INT(RND(1) * 10000))
LSET rec$ = a$
LSET rec2$ = MKI$(CVI(a$) + 1)
PUT #1, f
PUT #2, 213 - f
NEXT f
CLOSE #2
LSET rec$ = MKI$(VAL(level$) + 1)
PUT #1, 66
LSET rec$ = MKI$(VAL(level$) + 2)
PUT #1, 147
CLOSE #1
RETURN

'Подпрограмма ввода нового уровня
1300 IF curlev(lcursor) = 0 THEN GOTO 1600
COLOR 12: LOCATE 22, 16: PRINT "LEVEL:"; : COLOR 10
PRINT "_"; CHR$(29);
a$ = ""
1310 i$ = INKEY$
IF i$ = "" THEN GOTO 1310
IF ASC(i$) = 27 THEN GOSUB 1700: GOTO 20
IF ASC(i$) = 8 AND LEN(a$) > 0 THEN a$ = LEFT$(a$, LEN(a$) - 1): PRINT " "; CHR$(29); CHR$(29); "_"; CHR$(29);
IF ASC(i$) > 47 AND ASC(i$) < 58 AND LEN(a$) < 3 THEN a$ = a$ + i$: PRINT i$; "_"; CHR$(29);
IF ASC(i$) <> 13 THEN GOTO 1310
PRINT " "
level = VAL(a$)
IF level = 0 THEN GOSUB 1700: GOTO 20
IF level > curlev(lcursor) THEN LOCATE 23, 14: COLOR 12: PRINT "ACCESS DENIED": COLOR 10: PRINT TAB(8); "PRESS ANY KEY TO CONTINUE"; : WHILE INKEY$ = "": WEND: GOSUB 1700: GOTO 20
OPEN "soldier.tmp" FOR OUTPUT AS #1
PRINT #1, 1; ","; lcursor; ","; level; ","; gs&
CLOSE #1
IF level < 101 THEN RUN "game1.mod"

'Подпрограмма удаления игрока
1400 IF curlev(lcursor) = 0 THEN GOTO 20
COLOR 12: LOCATE 22, 12: PRINT "ARE YOU SURE?(Y/N)"
1410 i$ = INKEY$
IF i$ = "" THEN GOTO 1410
IF i$ <> "y" AND i$ <> "Y" THEN GOTO 1420
saves$(lcursor) = "--------": curlev(lcursor) = 0: LOCATE 9 + lcursor, 3: COLOR 15: PRINT saves$(lcursor); "  "; curlev(lcursor); "  ": KILL "gamesav" + RIGHT$(STR$(lcursor - 1), 1) + ".dat"
ON ERROR GOTO 2000
CHDIR "RECORDS"
KILL "rec" + RIGHT$(STR$(lcursor - 1), 1) + "???.dat"
CHDIR ".."
ON ERROR GOTO 0
1420 GOSUB 1700: GOTO 20

'Подпрограмма выбора скорости задержки игры
1500 COLOR 10: LOCATE 22, 8: PRINT "OLD GAME SPEED:"; RIGHT$(STR$(gs&), LEN(STR$(gs&)) - 1)
COLOR 12: LOCATE 23, 8: PRINT "NEW GAME SPEED:"; : COLOR 10
PRINT "_"; CHR$(29);
a$ = ""
1510 i$ = INKEY$
IF i$ = "" THEN GOTO 1510
IF ASC(i$) = 27 THEN GOSUB 1700: GOTO 20
IF ASC(i$) = 8 AND LEN(a$) > 0 THEN a$ = LEFT$(a$, LEN(a$) - 1): PRINT " "; CHR$(29); CHR$(29); "_"; CHR$(29);
IF ASC(i$) > 47 AND ASC(i$) < 58 AND LEN(a$) < 9 THEN a$ = a$ + i$: PRINT i$; "_"; CHR$(29);
IF ASC(i$) <> 13 THEN GOTO 1510
a& = VAL(a$)
IF a& = 0 THEN GOSUB 1700: GOTO 20
IF a& < 25 THEN a& = 25
gs& = a&
OPEN "soldier.cfg" FOR OUTPUT AS #1
PRINT #1, gs&; ","; ls&
CLOSE #1
GOSUB 1700: GOTO 20

'Подпрограмма создания нового игрока
1600 COLOR 12: LOCATE 22, 13: PRINT "NAME:"; : COLOR 10
PRINT "_"; CHR$(29);
a$ = ""
1610 i$ = INKEY$
IF i$ = "" THEN GOTO 1610
IF ASC(i$) = 27 THEN GOSUB 1700: GOTO 20
IF ASC(i$) = 8 AND LEN(a$) > 0 THEN a$ = LEFT$(a$, LEN(a$) - 1): PRINT " "; CHR$(29); CHR$(29); "_"; CHR$(29);
IF ASC(i$) > 96 AND ASC(i$) < 123 THEN i$ = CHR$(ASC(i$) - 32)
IF (ASC(i$) > 47 AND ASC(i$) < 58 OR ASC(i$) > 64 AND ASC(i$) < 91) AND LEN(a$) < 8 THEN a$ = a$ + i$: PRINT i$; "_"; CHR$(29);
IF ASC(i$) <> 13 THEN GOTO 1610
IF a$ = "" THEN GOSUB 1700: GOTO 20
FOR f = 1 TO 7
IF LEN(a$) = f THEN a$ = a$ + SPACE$(8 - f)
NEXT f
saves$(lcursor) = a$
curlev(lcursor) = 1
LOCATE 9 + lcursor, 3: COLOR 15: PRINT saves$(lcursor); "  "; curlev(lcursor); "  "
OPEN "gamesav" + RIGHT$(STR$(lcursor - 1), 1) + ".dat" FOR RANDOM AS #1 LEN = 1
FIELD #1, 1 AS rec$
FOR f = 1 TO 8
LSET rec$ = MID$(saves$(lcursor), f, 1)
PUT #1, f
NEXT f
FOR f = 9 TO 416
PUT #1, f
NEXT f
CLOSE #1
OPEN "gamesav" + RIGHT$(STR$(lcursor - 1), 1) + ".dat" FOR RANDOM AS #1 LEN = 2
FIELD #1, 2 AS rec$
OPEN "gamesav" + RIGHT$(STR$(lcursor - 1), 1) + ".dat" FOR RANDOM AS #2 LEN = 2
FIELD #2, 2 AS rec2$
FOR f = 5 TO 106
a$ = MKI$(INT(RND(1) * 10000))
LSET rec$ = a$
LSET rec2$ = MKI$(CVI(a$) + 1)
PUT #1, f
PUT #2, 213 - f
NEXT f
CLOSE #2
LSET rec$ = MKI$(1)
PUT #1, 66
LSET rec$ = MKI$(2)
PUT #1, 147
CLOSE #1
GOSUB 1700: GOTO 20

'Подпрограмма очистки нижней части экрана
1700 LOCATE 22, 1: PRINT TAB(39); : PRINT : PRINT TAB(39); : PRINT : PRINT TAB(39);
RETURN

'Подпрограмма ввода новой демонстрации
1800 IF curlev(lcursor) = 0 THEN GOTO 1600
COLOR 12: LOCATE 22, 16: PRINT "LEVEL:"; : COLOR 10
PRINT "_"; CHR$(29);
a$ = ""
1810 i$ = INKEY$
IF i$ = "" THEN GOTO 1810
IF ASC(i$) = 27 THEN GOSUB 1700: GOTO 20
IF ASC(i$) = 8 AND LEN(a$) > 0 THEN a$ = LEFT$(a$, LEN(a$) - 1): PRINT " "; CHR$(29); CHR$(29); "_"; CHR$(29);
IF ASC(i$) > 47 AND ASC(i$) < 58 AND LEN(a$) < 3 THEN a$ = a$ + i$: PRINT i$; "_"; CHR$(29);
IF ASC(i$) <> 13 THEN GOTO 1810
PRINT " "
level = VAL(a$)
IF level = 0 THEN GOSUB 1700: GOTO 20
IF level >= curlev(lcursor) THEN LOCATE 23, 16: COLOR 12: PRINT "NO RECORD": COLOR 10: PRINT TAB(8); "PRESS ANY KEY TO CONTINUE"; : WHILE INKEY$ = "": WEND: GOSUB 1700: GOTO 20
OPEN "soldier.tmp" FOR OUTPUT AS #1
PRINT #1, 2; lcursor; level; ls&
CLOSE #1
IF level < 101 THEN RUN "game1.mod"

'Подпрограмма выбора скорости задержки демонстрации
1900 COLOR 10: LOCATE 22, 7: PRINT "OLD REPLAY SPEED:"; RIGHT$(STR$(ls&), LEN(STR$(ls&)) - 1)
COLOR 12: LOCATE 23, 7: PRINT "NEW REPLAY SPEED:"; : COLOR 10
PRINT "_"; CHR$(29);
a$ = ""
1910 i$ = INKEY$
IF i$ = "" THEN GOTO 1910
IF ASC(i$) = 27 THEN GOSUB 1700: GOTO 20
IF ASC(i$) = 8 AND LEN(a$) > 0 THEN a$ = LEFT$(a$, LEN(a$) - 1): PRINT " "; CHR$(29); CHR$(29); "_"; CHR$(29);
IF ASC(i$) > 47 AND ASC(i$) < 58 AND LEN(a$) < 9 THEN a$ = a$ + i$: PRINT i$; "_"; CHR$(29);
IF ASC(i$) <> 13 THEN GOTO 1910
a& = VAL(a$)
IF a& = 0 THEN GOSUB 1700: GOTO 20
IF a& < 25 THEN a& = 25
ls& = a&
OPEN "soldier.cfg" FOR OUTPUT AS #1
PRINT #1, gs&; ","; ls&
CLOSE #1
GOSUB 1700: GOTO 20

'Подпрограмма исправления ошибки(нет ненужного файла)
2000 RESUME NEXT

