(define-library (scheme write)
  (export display write
          ;write-shared write-simple
          )
  (begin
    (##define define ##define)
    (define display ##display)
    (define write ##write)
    ;(define write-shared ##write-shared)
    ;(define write-simple ##write-simple)
    ))
