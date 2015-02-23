(define-library (scheme char)
  (export
    ;char-alphabetic?
    char-ci<=? char-ci<? char-ci=? char-ci>=? char-ci>?
    char-upcase char-downcase
    ;char-foldcase char-lower-case? char-upper-case? char-numeric?
    ;char-whitespace? digit-value
    string-ci<=? string-ci<? string-ci=? string-ci>=? string-ci>?
    string-upcase string-downcase string-foldcase)
  (begin
    (##define define ##define)
    ;(define char-alphabetic? ##char-alphabetic?)
    (define char-ci<=? ##char-ci<=?)
    (define char-ci<? ##char-ci<?)
    (define char-ci=? ##char-ci=?)
    (define char-ci>=? ##char-ci>=?)
    (define char-ci>? ##char-ci>?)
    (define char-upcase ##char-upcase)
    (define char-downcase ##char-downcase)
    ;(define char-foldcase ##char-foldcase)
    ;(define char-lower-case? ##char-lower-case?)
    ;(define char-upper-case? ##char-upper-case?)
    ;(define char-numeric? ##char-numeric?)
    ;(define char-whitespace? ##char-whitespace?)
    ;(define digit-value ##digit-value)
    (define string-ci<=? ##string-ci<=?)
    (define string-ci<? ##string-ci<?)
    (define string-ci=? ##string-ci=?)
    (define string-ci>=? ##string-ci>=?)
    (define string-ci>? ##string-ci>?)
    (define string-upcase ##string-upcase)
    (define string-downcase ##string-downcase)
    (define string-foldcase ##string-foldcase)))
