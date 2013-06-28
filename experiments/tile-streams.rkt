#lang racket

(require louDBus/unsafe)

(define gimpplus (loudbus-proxy "edu.grinnell.cs.glimmer.GimpDBus"
                                "/edu/grinnell/cs/glimmer/gimp"
                                "edu.grinnell.cs.glimmer.gimpplus"))

(loudbus-methods gimpplus)

(define stream (car (loudbus-call gimpplus 'tile-stream-new 1 2)))
stream
(define tile (loudbus-call gimpplus 'tile-stream-get stream))
tile
