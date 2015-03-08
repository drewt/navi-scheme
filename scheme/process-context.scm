;; Copyright 2014-2015 Drew Thoreson
;;
;; This Source Code Form is subject to the terms of the Mozilla Public
;; License, v. 2.0. If a copy of the MPL was not distributed with this
;; file, You can obtain one at http://mozilla.org/MPL/2.0/.

(define-library (scheme process-context)
  (export
    command-line emergency-exit exit
    get-environment-variable get-environment-variables)
  (begin
    (##define define ##define)
    (define command-line ##command-line)
    (define emergency-exit ##emergency-exit)
    (define exit ##exit)
    (define get-environment-variable ##get-environment-variable)
    (define get-environment-variables ##get-environment-variables)))
