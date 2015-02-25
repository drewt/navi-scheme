(define-library (scheme base)
  (export
    testing
    apply begin ;call/cc call-with-current-continuation
    call-with-values ;cond-expand
    define ;define-record-type define-syntax
    define-values ;do dynamic-wind 
    error error-object-irritants error-object-message error-object?
    ;file-error? read-error? features guard
    include include-ci lambda let let* ;let*-values let-syntax
    let-values ;letrec letrec* letrec-syntax
    make-parameter parameterize quasiquote quote raise raise-continuable set!
    ;syntax-error syntax-rules unless when
    unquote values with-exception-handler

    * + - / < <= = >= > ;abs ceiling complex denominator
    even? ;exact exact-integer-sqrt exact-integer? exact?
    ;expt floor floor-quotient floor-remainder floor/ gcd inexact inexact?
    ;integer->char integer? lcm min max modulo
    negative? ;number? numerator
    odd? positive? quotient ;rational rationalize real?
    remainder ;round square
    ;truncate truncate-quotient truncate-remainder truncase/ zero?

    and boolean=? boolean? case cond if not or

    ;char->integer
    char<=? char<? char=? char>=? char>? char?
    
    append assoc assq assv
    car cdr caar cadr cdar cddr cons for-each
    length list list->string list->vector list-copy list-ref list-set!
    list-tail list? make-list map member memq memv
    null? pair? reverse set-car! set-cdr!

    make-vector vector vector->list vector-copy vector-copy! vector-fill!
    vector-length vector-map vector-ref vector-set! vector? ;vector-for-each

    bytevector bytevector-append bytevector-copy bytevector-copy!
    bytevector-length bytevector-u8-ref bytevector-u8-set! bytevector?
    make-bytevector string->utf8 utf8->string

    make-string number->string string string->list string->number
    string->vector vector->string string-append string-copy string-copy!
    string-fill! substring string-for-each string-length string-map string-ref
    string-set! string<=? string<? string=? string>=? string>?

    ;binary-port call-with-port char-ready?
    close-input-port close-output-port close-port
    current-error-port current-input-port current-output-port
    ;flush-output-port get-output-bytevector
    get-output-string input-port-open? input-port? newline
    ;open-input-bytevector open-output-bytevector
    open-input-string open-output-string output-port-open? output-port?
    peek-char peek-u8 port? ;read-bytevector read-bytevector!
    read-char read-u8 ;read-line read-string textual-port u8-ready?
    ;write-bytevector write-string
    write-char write-u8

    ;string->symbol symbol->string symbol=? symbol?
    eof-object eof-object?
    ;procedure?

    eq? equal? eqv?)
  (begin

    ;; Builtins
    (##define define ##define)
    (define testing 1)
    (define * ##*)
    (define + ##+)
    (define - ##-)
    (define / ##/)
    (define < ##<)
    (define <= ##<=)
    (define = ##=)
    (define >= ##>=)
    (define > ##>)
    ;(define abs ##abs)
    (define and ##and)
    (define append ##append)
    (define apply ##apply)
    (define assoc ##assoc)
    (define assq ##assq)
    (define assv ##assv)
    (define begin ##begin)
    ;(define binary-port ##binary-port)
    (define boolean=? ##boolean=?)
    (define boolean? ##boolean?)
    (define bytevector ##bytevector)
    (define bytevector-append ##bytevector-append)
    (define bytevector-copy ##bytevector-copy)
    (define bytevector-copy! ##bytevector-copy!)
    (define bytevector-length ##bytevector-length)
    (define bytevector-u8-ref ##bytevector-u8-ref)
    (define bytevector-u8-set! ##bytevector-u8-set!)
    (define bytevector? ##bytevector?)
    ;(define call/cc ##call/cc)
    ;(define call-with-current-continuation call/cc)
    ;(define call-with-port ##call-with-port)
    (define call-with-values ##call-with-values)
    (define car ##car)
    (define cdr ##cdr)
    (define case ##case)
    ;(define ceiling ##ceiling)
    ;(define char->integer ##char->integer)
    ;(define char-ready? ##char-ready?)
    (define char<=? ##char<=?)
    (define char<? ##char<?)
    (define char=? ##char=?)
    (define char>=? ##char>=?)
    (define char>? ##char>?)
    (define char? ##char?)
    (define close-input-port ##close-input-port)
    (define close-output-port ##close-output-port)
    (define close-port ##close-port)
    ;(define complex? ##complex?)
    (define cond ##cond)
    ;(define cond-expand ##cond-expand)
    (define cons ##cons)
    (define current-error-port ##current-error-port)
    (define current-input-port ##current-input-port)
    (define current-output-port ##current-output-port)
    ;(define define-record-type ##define-record-type)
    ;(define define-syntax ##define-syntax)
    (define define-values ##define-values)
    ;(define denominator ##denominator)
    ;(define do ##do)
    ;(define dynamic-wind ##dynamic-wind)
    ;(define else ##else)
    (define eof-object ##eof-object)
    (define eof-object? ##eof-object?)
    (define eq? ##eq?)
    (define equal? ##equal?)
    (define eqv? ##eqv?)
    (define error ##error)
    (define error-object-irritants ##error-object-irritants)
    (define error-object-message ##error-object-message)
    (define error-object? ##error-object?)
    (define even? ##even?)
    ;(define exact ##exact)
    ;(define exact-integer-sqrt ##exact-integer-sqrt)
    ;(define exact-integer? ##exact-integer?)
    ;(define exact? ##exact?)
    ;(define expt ##expt)
    ;(define features ##features)
    ;(define file-error? ##file-error?)
    ;(define floor ##floor)
    ;(define floor-quotient ##floor-quotient)
    ;(define floor-remainder ##floor-remainder)
    ;(define floor/ ##floor/)
    ;(define flush-output-port ##flush-output-port)
    (define for-each ##for-each)
    ;(define gcd ##gcd)
    ;(define get-output-bytevector ##get-output-bytevector)
    (define get-output-string ##get-output-string)
    ;(define guard ##guard)
    (define if ##if)
    (define include ##include)
    (define include-ci ##include-ci)
    ;(define inexact ##inexact)
    ;(define inexact? ##inexact?)
    (define input-port-open? ##input-port-open?)
    (define input-port? ##input-port?)
    ;(define integer->char ##integer->char)
    ;(define integer? ##integer?)
    (define lambda ##lambda)
    ;(define lcm ##lcm)
    (define length ##length)
    (define let ##let)
    (define let* ##let*)
    ;(define let*-values ##let*-values)
    ;(define let-syntax ##let-syntax)
    (define let-values ##let-values)
    ;(define letrec ##letrec)
    ;(define letrec* ##letrec*)
    ;(define letrec-syntax ##letrec-syntax)
    (define list ##list)
    (define list->string ##list->string)
    (define list->vector ##list->vector)
    (define list-copy ##list-copy)
    (define list-ref ##list-ref)
    (define list-set! ##list-set!)
    (define list-tail ##list-tail)
    (define list? ##list?)
    (define make-bytevector ##make-bytevector)
    (define make-list ##make-list)
    (define make-parameter ##make-parameter)
    (define make-string ##make-string)
    (define make-vector ##make-vector)
    (define map ##map)
    ;(define max ##max)
    (define member ##member)
    (define memq ##memq)
    (define memv ##memv)
    ;(define min ##min)
    ;(define modulo ##modulo)
    (define negative? ##negative?)
    (define newline ##newline)
    (define not ##not)
    (define null? ##null?)
    (define number->string ##number->string)
    ;(define number? ##number?)
    ;(define numerator ##numerator)
    (define odd? ##odd?)
    ;(define open-input-bytevector ##open-input-bytevector)
    (define open-input-string ##open-input-string)
    ;(define open-output-bytevector ##open-output-bytevector)
    (define open-output-string ##open-output-string)
    (define or ##or)
    (define output-port-open? ##output-port-open?)
    (define output-port? ##output-port?)
    (define pair? ##pair?)
    (define parameterize ##parameterize)
    (define peek-char ##peek-char)
    (define peek-u8 ##peek-u8)
    (define port? ##port?)
    (define positive? ##positive?)
    ;(define procedure? ##procedure?)
    (define quasiquote ##quasiquote)
    (define quote ##quote)
    (define quotient ##quotient)
    (define raise ##raise)
    (define raise-continuable ##raise-continuable)
    ;(define rational? ##rational?)
    ;(define rationalize ##rationalize)
    ;(define read-bytevector ##read-bytevector)
    ;(define read-bytevector! ##read-bytevector!)
    (define read-char ##read-char)
    (define read-error? ##read-error?)
    ;(define read-line ##read-line)
    ;(define read-string ##read-string)
    (define read-u8 ##read-u8)
    ;(define real? ##real?)
    (define remainder ##remainder)
    (define reverse ##reverse)
    ;(define round ##round)
    (define set! ##set!)
    (define set-car! ##set-car!)
    (define set-cdr! ##set-cdr!)
    ;(define square ##square)
    (define string ##string)
    (define string->list ##string->list)
    (define string->number ##string->number)
    ;(define string->symbol ##string->symbol)
    (define string->utf8 ##string->utf8)
    (define string->vector ##string->vector)
    (define string-append ##string-append)
    (define string-copy ##string-copy)
    (define string-copy! ##string-copy!)
    (define string-fill! ##string-fill!)
    (define string-length ##string-length)
    (define string-ref ##string-ref)
    (define string-set! ##string-set!)
    (define string<=? ##string<=?)
    (define string<? ##string<?)
    (define string=? ##string=?)
    (define string>=? ##string>=?)
    (define string>? ##string>?)
    (define string? ##string?)
    (define substring ##substring)
    ;(define symbol->string ##symbol->string)
    ;(define symbol=? ##symbol=?)
    ;(define symbol? ##symbol?)
    ;(define syntax-error ##syntax-error)
    ;(define syntax-rules ##syntax-rules)
    ;(define textual-port? ##textual-port)
    ;(define truncate ##truncate)
    ;(define truncate-quotient ##truncate-quotient)
    ;(define truncate-remainder ##truncate-remainder)
    ;(define truncate/ ##truncate/)
    ;(define u8-ready? ##u8-ready?)
    ;(define unless ##unless)
    (define unquote ##unquote)
    ;(define unquote-splicing ##unquote-splicing)
    (define utf8->string ##utf8->string)
    (define values ##values)
    (define vector ##vector)
    (define vector->list ##vector->list)
    (define vector->string ##vector->string)
    ;(define vector-append ##vector-append)
    (define vector-copy ##vector-copy)
    (define vector-copy! ##vector-copy!)
    (define vector-fill! ##vector-fill!)
    ;(define vector-for-each ##vector-for-each)
    (define vector-length ##vector-length)
    (define vector-map ##vector-map)
    (define vector-ref ##vector-ref)
    (define vector-set! ##vector-set!)
    (define vector? ##vector?)
    ;(define when ##when)
    (define with-exception-handler ##with-exception-handler)
    ;(define write-bytevector ##write-bytevector)
    (define write-char ##write-char)
    ;(define write-string ##write-string)
    (define write-u8 ##write-u8)
    (define zero? ##zero?)

    ;; Non-builtins
    (define (caar pair) (car (car pair)))
    (define (cadr pair) (car (cdr pair)))
    (define (cdar pair) (cdr (car pair)))
    (define (cddr pair) (cdr (cdr pair)))
    (define (string-for-each proc str . strs)
      (apply for-each
             proc
             (string->list str)
             (map string->list strs)))
    (define (string-map proc str . strs)
      (list->string
        (apply map
               proc
               (string->list str)
               (map string->list strs))))))
