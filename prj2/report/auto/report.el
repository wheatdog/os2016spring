(TeX-add-style-hook
 "report"
 (lambda ()
   (TeX-add-to-alist 'LaTeX-provided-class-options
                     '(("article" "12pt")))
   (TeX-add-to-alist 'LaTeX-provided-package-options
                     '(("geometry" "top=1in" "bottom=1in" "left=1in" "right=1in")))
   (TeX-run-style-hooks
    "latex2e"
    "article"
    "art12"
    "xeCJK"
    "amsmath"
    "geometry"
    "fancyhdr"
    "hyperref")
   (TeX-add-symbols
    "hmwkID"
    "hmwkTitle"
    "hmwkDueDate"
    "hmwkUniversity"
    "hmwkClass"
    "hmwkClassAlias"
    "hmwkClassSection"
    "hmwkClassInstructor"
    "hmwkAuthorNameA"
    "hmwkAuthorNameB"
    "hmwkAuthorNameC")
   (LaTeX-add-bibitems
    "ManAptGet"))
 :latex)

