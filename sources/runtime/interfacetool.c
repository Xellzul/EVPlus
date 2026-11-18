/* interfacetool.c -- the interface tool's text variables and control-type registry (IT_Open registers BUTTON, IMAGE, BORDER ...). Notes: Research/notes/interfacetool.md */

#include "burgerplat.h"
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"

extern int32_t _IT_ParseAttributeGetAlign(unsigned char wide, const char *s);             /* itattr.c */
extern ITElement *_IT_ParseTreeFindParentTag(ITElement *root, ITElement *element,
                               const char *tag);                         /* itattr.c */
extern void _IT_ParseTreeGetFontAttributes(ITElement *root, ITElement *element, char **face, int32_t *size,
                         ULong *style, ITColor *color, char **href);     /* itattr.c */
extern unsigned char _IT_ParseAttributeGetFlag(unsigned char wide, const char *s);    /* itattr.c */
extern ITElement *_IT_ParseTreeFindThisData(ITElement *e, const char *text);          /* itattr.c */
extern ITElement *_IT_ParseTreeFindNextData(ITElement *root, ITElement *node);        /* itattr.c */

#define kVariablesOffset    ((int32_t)&((WindowNode *)0)->variables)

#define kBadIndex           -7001
#define kNoSuchControl      -7005

static ITRect gIT_EnableControlZeroRect;                      /* 0x0086bba0 IT_ViewSetItemEnabled's empty clip */
static ITRect gIT_ShowControlZeroRect;                      /* 0x0086bb90 IT_ViewSetItemVisible's empty clip */

#define INDEX_LEFT(i) (i--)

#define ParamWideArg int32_t

/* _IT_ViewParamText (set a text variable, replacing any old value, in a window's table or, for no
   window, the global one, created on first use). */
// FUNCTION: EVNOVA 0x0051f083
int32_t _IT_ViewParamText(WindowNode *window, const char *key, const char *value, ParamWideArg unused)
{
    struct HashTable **table;
    struct HashTable **alias;
    int32_t err, e;

    err = 0;
    table = 0;
    alias = 0;
    table = (struct HashTable **)window;
    if (table != 0)
        table = (struct HashTable **)((char *)table + kVariablesOffset);
    else
        table = (struct HashTable **)&gIT_DefaultHash;
    alias = table;
    if (*table == 0) {
        e = 1;
        if ((*table = HashCreate(1)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
    }
    while (HashRemove(*table, key))
        ;
    e = HashAppend(*table, key, value);
    if (e != 0)
        err = e;
done:
    return err;
}

/* IT_ViewParamText (set a text variable). */
// FUNCTION: EVNOVA 0x0051f0f2
int32_t IT_ViewParamText(WindowNode *window, const char *key, const char *value)
{
    return _IT_ViewParamText(window, key, value, 0);
}


/* IT_ViewGetItemRect (the bounds of the control at `index'; -7001 for a negative index, -7005 past the
   end). */
// FUNCTION: EVNOVA 0x0051f109
int32_t IT_ViewGetItemRect(void *windowRef, int32_t index, ITRect *r)
{
    int32_t err;
    WindowNode *window;
    ITControl *control;

    window = windowRef;
    err = 0;
    control = 0;
    if (index < 0) {
        err = kBadIndex;
        goto done;
    }
    DT_SetRect(r, 0, 0, 0, 0);
    for (control = window->controls->first; control != 0 && INDEX_LEFT(index); control = control->next)
        ;
    if (control == 0) {
        err = kNoSuchControl;
        goto done;
    }
    *r = control->bounds;
done:
    return err;
}

/* IT_ViewGetItemEnabled (whether the control at `index' is enabled, message 0x4a; -7001 for a negative
   index, -7005 past the end). */
// FUNCTION: EVNOVA 0x0051f16c
int32_t IT_ViewGetItemEnabled(void *windowRef, int32_t index, unsigned char *enabled)
{
    int32_t err;
    int32_t e;
    WindowNode *window;
    ITControl *control;

    window = windowRef;
    err = 0;
    control = 0;
    if (index < 0) {
        err = kBadIndex;
        goto done;
    }
    *enabled = 0;
    for (control = window->controls->first; control != 0 && INDEX_LEFT(index); control = control->next)
        ;
    if (control == 0) {
        err = kNoSuchControl;
        goto done;
    }
    e = _IT_ItemCallback(window, control, kMsgEnabled, 0, enabled);
    if (e != 0)
        err = e;
done:
    return err;
}

/* IT_ViewSetItemRect (move the control at `index' to `r': drawing clipped to nothing meanwhile, the old
   and new areas invalidated, message 0x41, the window refreshed; -7001 for a negative index,
   -7005 past the end). */
// FUNCTION: EVNOVA 0x0051f1cc
int32_t IT_ViewSetItemRect(void *windowRef, int32_t index, const ITRect *r)
{
    WindowNode *window = windowRef;
    int32_t err = 0;
    ITRect clip = {0, 0, 0, 0};
    void *saved;
    int32_t e;
    ITControl *control;
    ITRect area;

    saved = 0;
    control = 0;
    if (index < 0) {
        err = kBadIndex;
        goto done;
    }
    saved = DT_GetClipRegion(window->image);
    if (saved == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = DT_SetClipRect(window->image, &clip);
    if (e != 0) {
        err = e;
        goto done;
    }
    for (control = window->controls->first; control != 0 && INDEX_LEFT(index); control = control->next)
        ;
    if (control == 0) {
        err = kNoSuchControl;
        goto done;
    }
    if (window->image != 0) {
        area = control->bounds;
        DT_RectInset(&area, -4, -4);
        e = DT_AddInvalidRect(window->image, &area);
        if (e != 0) {
            err = e;
            goto done;
        }
        area = *r;
        DT_RectInset(&area, -4, -4);
        e = DT_AddInvalidRect(window->image, &area);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _IT_ItemCallback(window, control, kMsgBounds, (ULong)r, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    control->bounds = *r;
    if (saved != 0) {
        e = DT_SetClipRegion(window->image, saved);
        if (e != 0) {
            err = e;
            goto done;
        }
        DT_RegionDispose(saved);
        saved = 0;
    }
    e = _IT_ViewValidate(window);
    if (e != 0)
        err = e;
done:
    if (saved != 0) {
        DT_SetClipRegion(window->image, saved);
        DT_RegionDispose(saved);
    }
    return err;
}

/* IT_ViewSetItemVisible (show or hide the control at `index': drawing clipped to nothing meanwhile, its
   area invalidated, the focus moved off it first when it hides, its items rebuilt, the root's
   focusable list rebuilt, the focus taken when it shows and nothing has it; -7001 for a negative
   index, -7005 past the end). */
// FUNCTION: EVNOVA 0x0051f35e
int32_t IT_ViewSetItemVisible(void *windowRef, int32_t index, unsigned char show)
{
    WindowNode *window;
    ITRect clip;
    void *saved;
    int32_t err;
    int32_t e;
    ITControl *control;
    ITRect area;
    WindowNode *root;

    window = windowRef;
    err = 0;
    clip = gIT_ShowControlZeroRect;
    saved = 0;
    root = 0;
    control = 0;
    if (index < 0) {
        err = kBadIndex;
        goto done;
    }
    saved = DT_GetClipRegion(window->image);
    if (saved == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = DT_SetClipRect(window->image, &clip);
    if (e != 0) {
        err = e;
        goto done;
    }
    for (control = window->controls->first; control != 0 && INDEX_LEFT(index); control = control->next)
        ;
    if (control == 0) {
        err = kNoSuchControl;
        goto done;
    }
    if (window->image != 0) {
        area = control->bounds;
        DT_RectInset(&area, -4, -4);
        e = DT_AddInvalidRect(window->image, &area);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (!show && control == window->focus->control) {
        e = _IT_ViewMoveFocus(window, 1, 0, window->focus->control, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _IT_ItemCallback(window, control, kMsgShow, show ? 1 : 0, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IndirectSetSize(control->items, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = _IT_ItemCallback(window, control, kMsgItems, 0, control->items);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (saved != 0) {
        e = DT_SetClipRegion(window->image, saved);
        if (e != 0) {
            err = e;
            goto done;
        }
        DT_RegionDispose(saved);
        saved = 0;
    }
    for (root = window; root->parent != 0; root = root->parent)
        ;
    e = _IT_ViewQueryFocus(root, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (show && window->focus->control == 0) {
        e = _IT_ViewMoveFocus(window, 1, 0, 0, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _IT_ViewValidate(window);
    if (e != 0)
        err = e;
done:
    if (saved != 0) {
        DT_SetClipRegion(window->image, saved);
        DT_RegionDispose(saved);
    }
    return err;
}

/* IT_ViewSetItemEnabled (enable or disable the control at `index'; IT_ViewSetItemVisible's twin with message 0x4b). */
// FUNCTION: EVNOVA 0x0051f565
int32_t IT_ViewSetItemEnabled(void *windowRef, int32_t index, unsigned char enable)
{
    WindowNode *window;
    ITRect clip;
    void *saved;
    int32_t err;
    int32_t e;
    ITControl *control;
    ITRect area;
    WindowNode *root;

    window = windowRef;
    err = 0;
    clip = gIT_EnableControlZeroRect;
    saved = 0;
    root = 0;
    control = 0;
    if (index < 0) {
        err = kBadIndex;
        goto done;
    }
    saved = DT_GetClipRegion(window->image);
    if (saved == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = DT_SetClipRect(window->image, &clip);
    if (e != 0) {
        err = e;
        goto done;
    }
    for (control = window->controls->first; control != 0 && INDEX_LEFT(index); control = control->next)
        ;
    if (control == 0) {
        err = kNoSuchControl;
        goto done;
    }
    if (window->image != 0) {
        area = control->bounds;
        DT_RectInset(&area, -4, -4);
        e = DT_AddInvalidRect(window->image, &area);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (!enable && control == window->focus->control) {
        e = _IT_ViewMoveFocus(window, 1, 0, window->focus->control, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _IT_ItemCallback(window, control, kMsgEnable, enable ? 1 : 0, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IndirectSetSize(control->items, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = _IT_ItemCallback(window, control, kMsgItems, 0, control->items);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (saved != 0) {
        e = DT_SetClipRegion(window->image, saved);
        if (e != 0) {
            err = e;
            goto done;
        }
        DT_RegionDispose(saved);
        saved = 0;
    }
    for (root = window; root->parent != 0; root = root->parent)
        ;
    e = _IT_ViewQueryFocus(root, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (enable && window->focus->control == 0) {
        e = _IT_ViewMoveFocus(window, 1, 0, 0, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _IT_ViewValidate(window);
    if (e != 0)
        err = e;
done:
    if (saved != 0) {
        DT_SetClipRegion(window->image, saved);
        DT_RegionDispose(saved);
    }
    return err;
}

// FUNCTION: EVNOVA 0x0051f76c
int32_t IT_ItemHandlerInstall(WindowNode *owner, const char *name, ITControlProc proc)
{
    ControlType *type;
    int32_t err;

    err = 0;
    CHECK_WINDOWS(0);
    type = MemoryAllocateClear(sizeof(ControlType));
    if (type == 0) {
        err = kOutOfMemory;
    } else {
        String2UpperCopySafe(type->name, name, sizeof(type->name));
        type->proc = proc;
        if (owner != 0) {
            type->next = owner->controlTypes;
            owner->controlTypes = type;
        } else {
            type->next = gIT_HandlerList;
            gIT_HandlerList = type;
        }
    }
    return err;
}

/* IT_ItemHandlerLookup (a control type's procedure by name: the window's own types first, then the global
   ones; NULL when unknown). */
// FUNCTION: EVNOVA 0x0051f7ca
ITControlProc IT_ItemHandlerLookup(WindowNode *owner, const char *name)
{
    ControlType *type;
    char wanted[256];
    int32_t err, err2;

    err2 = err = 0;
    type = 0;
    CHECK_WINDOWS(0);
    String2UpperCopySafe(wanted, name, sizeof(wanted));
    if (owner != 0) {
        for (type = owner->controlTypes; type != 0; type = type->next) {
            if (StringCompare(wanted, type->name) == 0)
                break;
        }
    }
    if (type == 0) {
        for (type = gIT_HandlerList; type != 0; type = type->next) {
            if (StringCompare(wanted, type->name) == 0)
                break;
        }
    }
    return type != 0 ? type->proc : 0;
}

/* IT_ItemCallbackIsViewEnabled (whether the window a control proc serves and every window above it have
   WindowNode +0x25 and +0x26 set). */
// FUNCTION: EVNOVA 0x0051f855
unsigned char IT_ItemCallbackIsViewEnabled(void)
{
    int32_t err;
    WindowNode *w;
    unsigned char ok;

    err = 0;
    CHECK_WINDOWS(0);
    ok = 1;
    w = 0;
    ok = 1;
    for (w = gIT_CurrentView; ok && w != 0; w = w->parent)
        ok = w->enabled && w->visible;
    return ok;
}

/* IT_ItemCallbackSoundEvent (tell the served window's callback an event about the served control). */
// FUNCTION: EVNOVA 0x0051f884
int32_t IT_ItemCallbackSoundEvent(int32_t event)
{
    int32_t err;
    int32_t e;
    int32_t result;

    err = 0;
    result = 0;
    e = _IT_ViewCallback(gIT_CurrentView, 0, gIT_CurrentItem, event);
    if (e != 0)
        err = e;
    return err;
}

/* IT_ItemCallbackGrabFocus (give the focus to `item' of the served window's root, drawing clipped to the
   root's bounds meanwhile). */
// FUNCTION: EVNOVA 0x0051f8af
int32_t IT_ItemCallbackGrabFocus(int32_t item)
{
    int32_t err;
    void *saved;
    WindowNode *root;
    int32_t e;

    err = 0;
    CHECK_WINDOWS(0);
    saved = 0;
    root = 0;
    for (root = gIT_CurrentView; root->parent != 0; root = root->parent)
        ;
    saved = DT_GetClipRegion(root->image);
    if (saved == 0) {
        err = kOutOfMemory;
    } else {
        DT_SetClipRect(root->image, &root->bounds);
        e = _IT_ViewSetFocus(root, 0, item, 0);
        if (e != 0)
            err = e;
    }
    if (saved != 0) {
        DT_SetClipRegion(root->image, saved);
        DT_RegionDispose(saved);
    }
    return err;
}

/* IT_ItemCallbackDrawRect (repaint an area of the served window's root). */
// FUNCTION: EVNOVA 0x0051f924
int32_t IT_ItemCallbackDrawRect(void *unused, const ITRect *area)
{
    int32_t err;
    WindowNode *root;
    int32_t e;

    err = 0;
    root = 0;
    for (root = gIT_CurrentView; root->parent != 0; root = root->parent)
        ;
    e = _IT_ViewHandleDraw(root, area, 0);
    if (e != 0)
        err = e;
    return err;
}

/* IT_ItemCallbackGetParam (an attribute of the control a proc serves, or with `inherit' of its nearest
   enclosing element that has it, within the window's INTERFACE). */
// FUNCTION: EVNOVA 0x0051f955
char *IT_ItemCallbackGetParam(const char *key, unsigned char inherit)
{
    int32_t err = 0;
    char *result = 0;
    ITElement *root = 0;
    ITElement *element = 0;

    root = gIT_CurrentView->source.interfaceElement;
    element = gIT_CurrentItem->element;
    result = _IT_ParseTreeFindAttribute(root, element, 0, key, inherit);
    return result;
}

/* IT_ItemCallbackGetImage (an image by file name through the root window's preload cache: a cached one is
   answered at once, a pending or new one loaded from the page folder first (a new entry joins the
   cache); a missing file answers -7203 with no image). */
// FUNCTION: EVNOVA 0x0051f993
int32_t IT_ItemCallbackGetImage(const char *name, ITImage **image, ITImage **mask, ITTransparent *trans)
{
    int32_t err;
    ITPreload *p;
    unsigned char created;
    unsigned char flag;
    int32_t e;
    WindowNode *root;
    void **data;

    err = 0;
    p = 0;
    created = 0;
    flag = 0;
    data = (void **)(p = 0);
    root = 0;
    *image = (ITImage *)(root = 0);
    *mask = 0;
    DT_AlphaInfoDefaults(trans);
    for (root = gIT_CurrentView; root->parent != 0; root = root->parent)
        ;
    for (p = root->preloads; p != 0 && StringCompare(name, p->name) != 0; p = p->next)
        ;
    if (p == 0 || p->pending) {
        if (p == 0) {
            created = 1;
            p = MemoryAllocateClear(StringGetLength((const unsigned char *)name) + sizeof(ITPreload));
            if (p == 0) {
                err = kOutOfMemory;
                goto done;
            }
            StringCopy(p->name, name);
        }
        err = FT_FileGetFlags((int32_t)gIT_CurrentView->source.folder, p->name, 0, 0);
        if (err == 0) {
            e = FT_FileLoad((int32_t)gIT_CurrentView->source.folder, p->name, kChunkMax, 1, &data);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_ImageFileLoad(data, &p->image, &p->mask, &p->transparent, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            if (err != kFTErrFileNotFound)
                goto done;
            p->image = 0;
        }
    }
    if (p != 0) {
        if (created) {
            p->next = root->preloads;
            root->preloads = p;
        }
        p->stamp = TimerGetElapsedSeconds();
        p->pending = 0;
        *image = p->image;
        *mask = p->mask;
        MemoryCopy(trans, &p->transparent, sizeof(ITTransparent));
        p = 0;
    }
done:
    if (data != 0)
        IndirectDeallocate(data);
    if (err != 0 && created && p != 0) {
        if (p->image != 0)
            DT_ImageDispose(p->image);
        MemoryDeallocate(p);
    }
    return err;
}

/* IT_ItemCallbackGetNumber (a number attribute's value). */
// FUNCTION: EVNOVA 0x0051fb3a
int32_t IT_ItemCallbackGetNumber(const char *s)
{
    unsigned char ok;

    ok = 0;
    return _IT_ParseAttributeGetNumber(0, s);
}

/* IT_ItemCallbackGetColor (a colour attribute's value). */
// FUNCTION: EVNOVA 0x0051fb4c
ITColor IT_ItemCallbackGetColor(const char *s)
{
    unsigned char ok;

    ok = 0;
    return _IT_ParseAttributeGetColor(0, s);
}

/* IT_ItemCallbackGetFlag (a boolean attribute's value). */
// FUNCTION: EVNOVA 0x0051fb5e
unsigned char IT_ItemCallbackGetFlag(const char *s)
{
    unsigned char ok;

    ok = 0;
    return _IT_ParseAttributeGetFlag(0, s);
}

/* IT_ItemCallbackGetAlign (the served control's ALIGN, inherited up to its nearest CENTER element or the
   INTERFACE; whether there was one). */
// FUNCTION: EVNOVA 0x0051fb70
unsigned char IT_ItemCallbackGetAlign(int32_t *align)
{
    char *value;
    ITElement *root;
    ITElement *center;
    ITElement *element;
    int32_t err;
    unsigned char result;

    err = 0;
    root = 0;
    result = 0;
    value = 0;
    element = 0;
    center = 0;
    *align = 0;
    root = gIT_CurrentView->source.interfaceElement;
    element = gIT_CurrentItem->element;
    center = _IT_ParseTreeFindParentTag(root, element, "CENTER");
    if (center != 0)
        root = center;
    value = _IT_ParseTreeFindAttribute(root, element, 0, "ALIGN", 1);
    if (value != 0)
        *align = _IT_ParseAttributeGetAlign(0, value);
    return value != 0 ? true : false;
}

/* IT_ItemCallbackGetFont (the text style in force at the served control, 0x00540814). */
// FUNCTION: EVNOVA 0x0051fbed
void IT_ItemCallbackGetFont(char **face, int32_t *size, ULong *style, ITColor *color, char **href)
{
    int32_t err = 0;
    int32_t e = 0;
    ITElement *root = 0;
    ITElement *element = 0;

    root = gIT_CurrentView->source.interfaceElement;
    element = gIT_CurrentItem->element;
    _IT_ParseTreeGetFontAttributes(root, element, face, size, style, color, href);
}

/* IT_ItemCallbackGetNextRun (the served control's next non-empty text after the text `after', with the
   style in force there and its parent's order; NULL past the last). */
// FUNCTION: EVNOVA 0x0051fc33
char *IT_ItemCallbackGetNextRun(const char *after, char **face, int32_t *size, ULong *style, ITColor *color,
                   char **href, ULong *order)
{
    int32_t err = 0;
    int32_t e = 0;
    ITElement *root = 0;
    ITElement *node = 0;
    ITElement *element = 0;
    ITElement *t;

    t = element = 0;
    if (face != 0)
        *face = 0;
    if (size != 0)
        *size = 0;
    if (style != 0)
        *style = 0;
    if (href != 0)
        *href = 0;
    if (order != 0)
        *order = 0;
    element = gIT_CurrentItem->element;
    t = after != 0 ? _IT_ParseTreeFindThisData(element, after) : element;
    node = element->child != 0 ? _IT_ParseTreeFindNextData(element->child, t) : 0;
    if (node != 0 && (face != 0 || size != 0 || style != 0 || color != 0 || href != 0)) {
        root = gIT_CurrentView->source.interfaceElement;
        _IT_ParseTreeGetFontAttributes(root, node, face, size, style, color, href);
    }
    if (node != 0 && order != 0) {
        root = gIT_CurrentView->source.interfaceElement;
        *order = PT_ParseTreeGetParent(root, node)->order;
    }
    return node != 0 ? (char *)node->attributes : 0;
}

/* IT_ItemCallbackGetNextNode (the served control's next node in document order after `after', with the
   style in force there and its parent's order; NULL past the last). */
// FUNCTION: EVNOVA 0x0051fd3d
ITElement *IT_ItemCallbackGetNextNode(ITElement *after, char **face, int32_t *size, ULong *style,
                        ITColor *color, char **href, ULong *order)
{
    int32_t err = 0;
    int32_t e = 0;
    ITElement *root = 0;
    ITElement *node = 0;
    ITElement *element = 0;
    ITElement *t = 0;

    if (face != 0)
        *face = 0;
    if (size != 0)
        *size = 0;
    if (style != 0)
        *style = 0;
    if (href != 0)
        *href = 0;
    if (order != 0)
        *order = 0;
    element = gIT_CurrentItem->element;
    t = after != 0 ? after : element;
    node = element->child != 0 ? PT_ParseTreeGetNext(element->child, t) : 0;
    if (node != 0 && (face != 0 || size != 0 || style != 0 || color != 0 || href != 0)) {
        root = gIT_CurrentView->source.interfaceElement;
        _IT_ParseTreeGetFontAttributes(root, node, face, size, style, color, href);
    }
    if (node != 0 && order != 0) {
        root = gIT_CurrentView->source.interfaceElement;
        t = PT_ParseTreeGetParent(root, node);
        *order = t->order;
    }
    return node;
}
