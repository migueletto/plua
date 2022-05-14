#define MIN_RESOURCE    128
#define NUM_RESOURCE    128

typedef struct {
  UInt32 type;
  MemHandle h;
  MemPtr p;
  UInt16 size;
} ResourceType;

int resource_list(lua_State *L);
int resource_get(lua_State *L);
int resource_size(lua_State *L);
int resource_md5(lua_State *L);
int resource_call (lua_State *L);
ResourceType *resource_valid (lua_State *L);
