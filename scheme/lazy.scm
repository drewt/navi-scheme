(define-library (scheme lazy)
  (export
    delay
    ;delay-force
    force make-promise promise?)
  (begin
    (##define define ##define)
    (define delay ##delay)
    ;(define delay-force ##delay-force)
    (define force ##force)
    (define make-promise ##make-promise)
    (define promise? ##promise?)))
