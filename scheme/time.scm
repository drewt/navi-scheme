(define-library (scheme time)
  (export current-second current-jiffy jiffies-per-second)
  (begin
    (##define define ##define)
    (define current-second ##current-second)
    (define current-jiffy ##current-jiffy)
    (define jiffies-per-second ##jiffies-per-second)))
