#ifndef GraphingBase_h
#define GraphingBase_h

class GraphBase {
public:
    virtual void drawLocal(bool local_sizing = true) = 0;
    virtual void drawFresh(bool local_sizing = true) = 0;
    virtual void drawCursor(bool initial = false) = 0;
    virtual void dynamicPan(int8_t step) = 0;
    virtual void dynamicCursor(int8_t step) = 0;
    virtual void annotate(bool dayscale = true) = 0;
    virtual void drawLogos(enum screens screen, bool high) = 0;

    virtual ~GraphBase() {}
};

#endif
