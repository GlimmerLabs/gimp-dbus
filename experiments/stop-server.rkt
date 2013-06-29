#lang racket
(require louDBus/unsafe)

(define gimpplus (loudbus-proxy "edu.grinnell.cs.glimmer.GimpDBus"
                                "/edu/grinnell/cs/glimmer/gimp"
                                "edu.grinnell.cs.glimmer.gimpplus"))

(loudbus-methods gimpplus)

(loudbus-call gimpplus 'ggimp-about)
(loudbus-call gimpplus 'ggimp-quit)
