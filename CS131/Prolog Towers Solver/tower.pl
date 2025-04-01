ntower(N, T, C) :- 
    correctNumOfLists(T, N),
    nElementsPerList(T, N),
    C = counts(Top, Bottom, Left, Right),
    validNumbers(N, T),
    maplist(fd_labeling, T),
    transpose(T, TransposedT),
    maplist(fd_labeling, TransposedT),
    %now we have instantiated a valid grid and its transpose, and we only have towercheck left using counts
    checkLefttoRight(T, Left),
    checkRighttoLeft(T, Right),
    checkLefttoRight(TransposedT, Top), %looking from the left at TransposedT is same view as from the top at T
    checkRighttoLeft(TransposedT, Bottom). 

correctNumOfLists(T, N) :- length(T, N).

nElementsPerList([], _).
nElementsPerList([H|T], N) :- length(H, N), nElementsPerList(T, N).

validNumbers(N, T) :-
    eachNumWithinRange(N, T),
    eachRowUnique(T),
    eachColUnique(T).

eachNumWithinRange(_, []).
eachNumWithinRange(N, [H|T]) :- fd_domain(H, 1, N), eachNumWithinRange(N, T).

eachRowUnique([]).
eachRowUnique([H|T]) :- fd_all_different(H), eachRowUnique(T).

eachColUnique(T) :- transpose(T, T1), eachRowUnique(T1).

transpose([], []).
transpose([H|T], M) :- transpose(H, [H|T], M).
transpose([], _, []).
transpose([_|RestOfRow], A, [ResH|ResT]) :- 
    checkHeadsExtractTails(A, ResH, M), 
    transpose(RestOfRow, M, ResT).
checkHeadsExtractTails([], [], []).
checkHeadsExtractTails([[FirstElem|RestOfRow]|RemRows], [FirstElem|R], [RestOfRow|T]) :- checkHeadsExtractTails(RemRows, R, T).

checkLefttoRight([], []).
checkLefttoRight([Row1|RemRows], [Num1|RemNums]) :- 
    countRow(Row1, Num1), 
    checkLefttoRight(RemRows, RemNums).

checkRighttoLeft([], []).
checkRighttoLeft([Row1|RemRows], [Num1|RemNums]) :- 
    reverse(Row1, RevR1),  
    countRow(RevR1, Num1), 
    checkRighttoLeft(RemRows, RemNums).

countRow([], 0).
countRow([_], 1).
countRow(R, N) :- 
    numberSeen(0, R, OnlyVisibles), %min num to be seen, our Row, and Row consisting of only visible towers
    length(OnlyVisibles, OVLEN),
    N #= OVLEN.

numberSeen(_, [], []). 
numberSeen(MinHeight, [H|OurTail], [H|BuiltTail]) :- 
    H #> MinHeight, 
    numberSeen(H, OurTail, BuiltTail).
numberSeen(MinHeight, [H|OurTail], BuiltL) :- 
    H #< MinHeight,
    numberSeen(MinHeight, OurTail, BuiltL).
    




plain_ntower(N, T, C) :-
    correctNumOfLists(T, N),
    C = counts(Top, Bottom, Left, Right),
    formDomain(N, D), 
    pCheckLefttoRight(N, T, Left, Right, D),
    transpose(T, TransposedT),
    pCheckLefttoRight(N, TransposedT, Top, Bottom, D).

formDomain(N, D) :- 
    findall(X, between(1, N, X), D).

pCheckLefttoRight(_, [], [], [], _).
pCheckLefttoRight(N, [Row1|RemRows], [L1|RemL], [R1|RemR], D) :-
    length(Row1, N), %ensures each row is Nsized
    permutation(Row1, D), %valid numbers in each row
    pCountRow(Row1, L1),
    reverse(Row1, Row1Revd),
    pCountRow(Row1Revd, R1),
    pCheckLefttoRight(N, RemRows, RemL, RemR, D).

pCountRow(R, TowersVisible) :- 
    pNumberSeen(R, 0, 0, TowersVisible). %processing R, currentTallestHeight is 0, towersWeveSeen is 0, towersWeNeedToSee is TowersVisible

pNumberSeen([], _, TV, TV). % end of row, seen what we need to see
pNumberSeen([H|T], MinHeight, TSeen, TNeeded) :-
    (H > MinHeight)
    -> NewTSeen is TSeen + 1,
       pNumberSeen(T, H, NewTSeen, TNeeded)
    ;  pNumberSeen(T, MinHeight, TSeen, TNeeded).






speedup(R) :-
    time_ntower(T1),
    time_plain_ntower(T2), 
    R is T2/T1.

time_ntower(T) :- 
    statistics(cpu_time, [Begin|_]),
    ntower(5, _, counts([4,3,3,2,1],[1,2,3,2,3],[5,3,2,2,1],[1,2,3,_,_])),
    statistics(cpu_time, [End|_]),
    T is (End - Begin).

time_plain_ntower(T) :- 
    statistics(cpu_time, [Begin|_]),
    plain_ntower(5, _, counts([4,3,3,2,1],[1,2,3,2,3],[5,3,2,2,1],[1,2,3,_,_])),
    statistics(cpu_time, [End|_]),
    T is (End - Begin).





ambiguous(N, C, T1, T2) :-
    ntower(N, T1, C),
    ntower(N, T2, C),
    T1 \= T2.