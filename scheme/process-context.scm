(define-library (scheme process-context)
  (export
    command-line emergency-exit exit
    ;get-environment-variable get-environment-variables
    )
  (begin
    (##define define ##define)
    (define command-line ##command-line)
    (define emergency-exit ##emergency-exit)
    (define exit ##exit)
    ;(define get-environment-variable ##get-environment-variable)
    ;(define get-environment-variables ##get-environment-variables)
    ))
