;; Copyright 2014-2015 Drew Thoreson
;;
;; This Source Code Form is subject to the terms of the Mozilla Public
;; License, v. 2.0. If a copy of the MPL was not distributed with this
;; file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
