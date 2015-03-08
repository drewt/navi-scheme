;; Copyright 2014-2015 Drew Thoreson
;;
;; This Source Code Form is subject to the terms of the Mozilla Public
;; License, v. 2.0. If a copy of the MPL was not distributed with this
;; file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
