#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#undef uiControl
#undef uiButton
#undef uiLabel

#define uiControl(this) _Generic((this), \
      uiButton *: (uiControl *)this, \
      uiBox *: (uiControl *)this, \
      uiWindow *: (uiControl *)this, \
      uiProgressBar *: (uiControl *)this, \
      uiGroup *: (uiControl *)this, \
      uiLabel *: (uiControl *)this, \
      uiMultilineEntry *: (uiControl *)this, \
      uiTab *: (uiControl *)this, \
      uiSeparator *: (uiControl *)this, \
      uiControl *: this \
      )

#define uiButton(this) _Generic((this), \
      uiControl *: (uiButton *)this)

#define uiLabel(this) _Generic((this), \
      uiControl *: (uiLabel *)this)

#endif
