#lang racket
(provide (all-defined-out))

(define (is-lambda? sym)
  (member sym '(lambda λ)))

(define (expr-compare x y)
  (cond 
    [(equal? x y) x] ; exact same contents
    [(and (boolean? x) (boolean? y)) ; both booleans
        (if x '% '(not %))] 
    [(or (not (list? x)) (not (list? y))) ; at least one arg isnt a list
        (list 'if '% x y)] 
    [(and (list? x) (list? y) (not (equal? (length x) (length y)))) ; both lists, diff lengths
        (list 'if '% x y)]
    [(and (list? x) (list? y) (equal? (length x) (length y))) ; both lists, same length, call helper
        (process-heads x y)]))

(define (process-heads l1 l2)
  (cond
    [(and (equal? (car l1) 'if) (equal? (car l2) 'if)) ; both heads are 'if 
        (simple-list-cmp l1 l2)]
    [(or (equal? (car l1) 'if) (equal? (car l2) 'if)) ; one head is 'if, one isnt
        (list 'if '% l1 l2)]
    [(or (equal? (car l1) 'quote) (equal? (car l2) 'quote)) ; at least one head is quote
        (list 'if '% l1 l2)]
    [(and (equal? (car l1) 'lambda) (equal? (car l2) 'lambda)) ; boths heads lambda
        (cond
            [(not (equal? (length (cadr l1)) (length (cadr l2))))
                (list 'if '% l1 l2)]
            [else (process-lambda-caller (cdr l1) (cdr l2) 'lambda '() '())])]
    [(and (equal? (car l1) 'λ) (equal? (car l1) (car l2))) ; same as above case but with λ
        (cond
            [(not (equal? (length (cadr l1)) (length (cadr l2))))
                (list 'if '% l1 l2)]
            [else (process-lambda-caller (cdr l1) (cdr l2) 'λ '() '())])]
    [(and (is-lambda? (car l1)) (is-lambda? (car l2))) ; both heads are some lambda 
        (cond
            [(not (equal? (length (cadr l1)) (length (cadr l2))))
                (list 'if '% l1 l2)]
            [else (process-lambda-caller (cdr l1) (cdr l2) 'λ '() '())])]
    [(or (is-lambda? (car l1)) (is-lambda? (car l2))) ; one head is a lambda 
        (list 'if '% l1 l2)]
    [else (simple-list-cmp l1 l2)])) ; fallthrough

(define (simple-list-cmp l1 l2)
  (cond 
    [(and (empty? l1) (empty? l2)) '()]
    [(equal? (car l1) (car l2)) ; heads equal
        (cons (car l1) (simple-list-cmp (cdr l1) (cdr l2)))] ; keep head, cmp rest
    [(and (boolean? (car l1)) (boolean? (car l2))) ; heads both boolean
        (cons (if (car l1) '% '(not %)) (simple-list-cmp (cdr l1) (cdr l2)))] ; essentially % for x, not % for y, cmp rest
    [(and (list? (car l1)) (list? (car l2)) (equal? (length (car l1)) (length (car l2)))) ; heads are equal length lists
        (cons (process-heads (car l1) (car l2)) (simple-list-cmp (cdr l1) (cdr l2)))]
    [else
        (cons (list 'if '% (car l1) (car l2)) (simple-list-cmp (cdr l1) (cdr l2)))]))

(define (process-lambda-caller l1 l2 lam d1 d2)
  (list lam
      (process-args (car l1) (car l2))
      (process-body (cadr l1) (cadr l2)
          (cons (build-dict1 (car l1) (car l2)) d1)
          (cons (helper-build-dict2 (car l1) (car l2)) d2))))

(define (process-args al1 al2)
  (cond 
      [(and (empty? al1) (empty? al2)) '()]
      [(equal? (car al1) (car al2)) ; head args equal
          (cons (car al1) (process-args (cdr al1) (cdr al2)))]
      [else
          (cons (string->symbol ; cons (headarg1!headarg2 tailsresult)
              (string-append (symbol->string (car al1)) "!" (symbol->string (car al2)))
              )
              (process-args (cdr al1) (cdr al2)))]))

(define (process-body e1 e2 d1 d2)
  (let ([e1-current (if (equal? (curVal-from-dicts e1 d1) "E N/A") e1 (curVal-from-dicts e1 d1))]
        [e2-current (if (equal? (curVal-from-dicts e2 d2) "E N/A") e2 (curVal-from-dicts e2 d2))])
    (cond
        [(and (list? e1) (list? e2) (equal? (length e1) (length e2))) ; both same length lists
            (process-body-list e1 e2 d1 d2)]
        [(equal? e1-current e2-current) ; both equal
            e1-current]
        [(and (boolean? e1) (boolean? e2)) ; both booleans
            (if e1 '% '(not %))]
        [(or (not (list? e1)) (not (list? e2))) ; at least one is not a list
            (list 'if '% (if (list? e1) (modify-expression e1 d1 #t) e1-current)
                         (if (list? e2) (modify-expression e2 d2 #t) e2-current))]
        [(and (list? e1) (list? e2) (not (equal? (length e1) (length e2)))) ; diff length lists
            (list 'if '% (modify-expression e1 d1 #t) (modify-expression e2 d2 #t))])))

(define (process-body-list l1 l2 d1 d2)
  (cond
      [(and (equal? (car l1) 'if) (equal? (car l2) 'if)) ; both heads 'if
          (cons 'if (process-lists-in-lambdas (cdr l1) (cdr l2) d1 d2))]
      [(or (equal? (car l1) 'if) (equal? (car l2) 'if)) ; one head 'if
          (list 'if '% (modify-expression l1 d1 #t) (modify-expression l2 d2 #t))]
      [(or (equal? (car l1) 'quote) (equal? (car l2) 'quote)) ; some head is quote
          (if (equal? l1 l2) l1 
                            (list 'if '% (modify-expression l1 d1 #t) (modify-expression l2 d2 #t)))]
      [(and (equal? (car l1) 'lambda) (equal? (car l2) 'lambda)) ; both heads lambda
          (cond
              [(not (equal? (length (cadr l1)) (length (cadr l2)))) 
                (list 'if '% (modify-expression l1 d1 #t) (modify-expression l2 d2 #t))]
              [else (process-lambda-caller (cdr l1) (cdr l2) 'lambda d1 d2)])]
      [(and (equal? (car l1) 'λ) (equal? (car l1) 'λ)) ; same as lambda but for λ
          (cond
              [(not (equal? (length (cadr l1)) (length (cadr l2))))
                (list 'if '% (modify-expression l1 d1 #t) (modify-expression l2 d2 #t))]
              [else (process-lambda-caller (cdr l1) (cdr l2) 'λ d1 d2)])]
      [(and (is-lambda? (car l1)) (is-lambda? (car l2))) ; one head lambda, one λ
          (cond
              [(not (equal? (length (cadr l1)) (length (cadr l2))))
                (list 'if '% (modify-expression l1 d1 #t) (modify-expression l2 d2 #t))]
              [else (process-lambda-caller (cdr l1) (cdr l2) 'λ d1 d2)])]
      [(or (is-lambda? (car l1)) (is-lambda? (car l2))) ; one head is lambda or λ
          (list 'if '% (modify-expression l1 d1 #t) (modify-expression l2 d2 #t))]
      [else (process-lists-in-lambdas l1 l2 d1 d2)]))

(define (process-lists-in-lambdas l1 l2 d1 d2)
  (if (and (empty? l1) (empty? l2)) '()
  (let ([cur1 (if (equal? (curVal-from-dicts (car l1) d1) "E N/A") (car l1) (curVal-from-dicts (car l1) d1))]
        [cur2 (if (equal? (curVal-from-dicts (car l2) d2) "E N/A") (car l2) (curVal-from-dicts (car l2) d2))])
    (cond
        [(and (list? cur1) (list? cur2)) ; both lists
          (cond
            [(equal? (length (car l1)) (length (car l2)))
                (cons (process-body-list (car l1) (car l2) d1 d2) (process-lists-in-lambdas (cdr l1) (cdr l2) d1 d2))]
            [else
                (cons (list 'if '% (modify-expression (car l1) d1 #t) (modify-expression (car l2) d2 #t)) (process-lists-in-lambdas (cdr l1) (cdr l2) d1 d2))])]
        [(or (list? cur1) (list? cur2)) ; one is a list
            (list 'if '% (if (list? l1) (modify-expression l1 d1 #t) cur1) (if (list? l2) (modify-expression l2 d2 #t) cur2))]
        [(equal? cur1 cur2)
            (cons cur1 (process-lists-in-lambdas (cdr l1) (cdr l2) d1 d2))]
        [(and (boolean? (car l1)) (boolean? (car l2))) ; both booleans
            (cons (if (car l1) '% '(not %)) (process-lists-in-lambdas (cdr l1) (cdr l2) d1 d2))]
        [else
            (cons (list 'if '% cur1 cur2) (process-lists-in-lambdas (cdr l1) (cdr l2) d1 d2))]))))

(define (modify-expression symbolList dList hd)
  (if (list? symbolList)
  (cond
      [(empty? symbolList) '()]
      [(equal? (car symbolList) 'quote) symbolList] ; don't mess with stuff in quotes
      [(and hd (is-lambda? (car symbolList)))
          (cons (car symbolList) (cons (cadr symbolList) ; keep first 2 elems, rebuild dictionary list and process tail
          (modify-expression (cddr symbolList) (cons (build-dict1 (cadr symbolList) (cadr symbolList)) dList) #f)))]
      [(and hd (equal? (car symbolList) 'if))
          (cons (car symbolList) (modify-expression (cdr symbolList) dList #f))]
      [(boolean? (car symbolList))
          (cons (car symbolList) (modify-expression (cdr symbolList) dList #f))]    
      [(list? (car symbolList))
          (cons (modify-expression (car symbolList) dList #t) (modify-expression (cdr symbolList) dList #f))]
      [else
        (cons (if (equal? (curVal-from-dicts (car symbolList) dList) "E N/A")
                  (car symbolList)
                  (curVal-from-dicts (car symbolList) dList))
              (modify-expression (cdr symbolList) dList #f))]
  )
  symbolList))

(define (curVal-from-dicts v dicts)
  (cond
      [(empty? dicts) "E N/A"]
      [(not (equal? (hash-ref (car dicts) v "E N/A") "E N/A"))
          (hash-ref (car dicts) v "E N/A")]
      [else (curVal-from-dicts v (cdr dicts))]))

(define (build-dict1 l1 l2)
  (cond
      [(and (empty? l1) (empty? l2)) 
          (hash)]
      [(equal? (car l1) (car l2))
          (hash-set (build-dict1 (cdr l1) (cdr l2)) (car l1) (car l1))]
      [else
          (hash-set (build-dict1 (cdr l1) (cdr l2))
            (car l1)
            (string->symbol (string-append (symbol->string (car l1)) "!" (symbol->string (car l2)))))]))

(define (helper-build-dict2 l1 l2)
  (cond
      [(and (empty? l1) (empty? l2)) 
          (hash)]
      [(equal? (car l1) (car l2))
          (hash-set (helper-build-dict2 (cdr l1) (cdr l2)) (car l2) (car l2))]
      [else
          (hash-set (helper-build-dict2 (cdr l1) (cdr l2))
            (car l2)
            (string->symbol (string-append (symbol->string (car l1)) "!" (symbol->string (car l2)))))]))                                               

(define (test-expr-compare x y)
  (and
   (equal? (eval x) (eval (list 'let '([% #t]) (expr-compare x y))))
   (equal? (eval y) (eval (list 'let '([% #f]) (expr-compare x y))))))

(define test-expr-x
  '(lambda (x y) (lambda (z) (if x (quote (x y)) (lambda (a1 a2 a3) (+ a1 x a3))))))
(define test-expr-y
  '(lambda (x y) (lambda (f) (if f '(a b) (lambda (a1 a2) (+ a1 if x))))))