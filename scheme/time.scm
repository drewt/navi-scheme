;; Copyright 2014-2015 Drew Thoreson
;;
;; This Source Code Form is subject to the terms of the Mozilla Public
;; License, v. 2.0. If a copy of the MPL was not distributed with this
;; file, You can obtain one at http://mozilla.org/MPL/2.0/.

(define-library (scheme time)
  (export current-second current-jiffy jiffies-per-second)
  (begin
    (##define define ##define)
    (define current-second ##current-second)
    (define current-jiffy ##current-jiffy)
    (define jiffies-per-second ##jiffies-per-second)))
