#lang racket

; A few basic experiments with tile streams.

(require louDBus/unsafe)

(define gimpplus (loudbus-proxy "edu.grinnell.cs.glimmer.GimpDBus"
                                "/edu/grinnell/cs/glimmer/gimp"
                                "edu.grinnell.cs.glimmer.gimpplus"))

(loudbus-methods gimpplus)

; Build a stream
(display "Building a stream: ")
(define stream (car (loudbus-call gimpplus 'tile-stream-new 1 2)))
(display stream)
(newline)

; Check if it's valid
(display "Checking validity: ")
(display (loudbus-call gimpplus 'tile-stream-is-valid stream))
(newline)

; Check if a random integer is a valid string
(display "Checking validity of 123: ")
(display (loudbus-call gimpplus 'tile-stream-is-valid 123))
(newline)

; Forge ahead in any case, grabbing the tile
(display "First tile: ")
(define tile1 (loudbus-call gimpplus 'tile-stream-get stream))
(display (cons (car tile1) 
               (cons (subbytes (cadr tile1) 0 10)
                     (cddr tile1))))
(newline)

; Remember the bytes
(define bytes1 (cadr tile1))

; Try the next tile
(display "Advancing: ")
(display (loudbus-call gimpplus 'tile-stream-advance stream))
(newline)

; Forge ahead in any case, grabbing second tile
(display "Second tile: ")
(define tile2 (loudbus-call gimpplus 'tile-stream-get stream))
(display (cons (car tile2) 
               (cons (subbytes (cadr tile2) 0 10)
                     (cddr tile2))))
(newline)

; Copy bytes from the first tile
(display "Updating tile: ")
(display (loudbus-call gimpplus 'tile-update stream
                       (bytes-length bytes1) bytes1))
(newline)

; Close the stream
(display "Closing: ")
(display (loudbus-call gimpplus 'tile-stream-close stream))
(newline)
