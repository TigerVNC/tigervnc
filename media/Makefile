all: icons

.PHONY: clean
clean: clean-icons

ICONS := \
    icons/tigervnc_16.png \
    icons/tigervnc_22.png \
    icons/tigervnc_24.png \
    icons/tigervnc_32.png \
    icons/tigervnc_48.png

.PHONY: icons clean-icons
icons: $(ICONS)

clean-icons:
	rm -f $(ICONS)

icons/tigervnc_16.png: tigervnc_16.svg
	convert -background transparent "$<" "$@"

icons/tigervnc_22.png: tigervnc_20.svg
	convert -size 22x22 xc:none -gravity center -background transparent "$<" -composite "$@"
icons/tigervnc_24.png: tigervnc_20.svg
	convert -size 24x24 xc:none -gravity center -background transparent "$<" -composite "$@"
icons/tigervnc_32.png: tigervnc_28.svg
	convert -size 32x32 xc:none -gravity center -background transparent "$<" -composite "$@"
icons/tigervnc_48.png: tigervnc_42.svg
	convert -size 48x48 xc:none -gravity center -background transparent "$<" -composite "$@"

