typedef enum {
  gadgetDraw, gadgetErase, gadgetDown, gadgetUp, gadgetMove, gadgetDelete,
  gadgetGetText, gadgetSetText, gadgetGetState, gadgetSetState
} GadgetEvent;

typedef Boolean (*GadgetCallback)(void *context, GadgetEvent event, int id, void *data, void *arg, RectangleType *rect);
