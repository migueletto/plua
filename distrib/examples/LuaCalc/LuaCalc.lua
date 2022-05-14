-- LuaCalc.lua
-- Copyright (c) Marcio M. Andrade

-- This program requires the library 3dlib2.prc

-- Stack definition

stack = { index = 0 }

function stack:push(v)
  self[self.index] = v
  self.index = self.index + 1
end

function stack:pop()
  if self.index > 0 then
    self.index = self.index - 1
    return self[self.index]
  end
  return 0
end

-- Display definition

display = {}

function display:init(x,y,w,h,fg,bg)
  self.value = 0
  self.max = 15
  self.x = x
  self.y = y
  self.w = w
  self.h = h
  self.fg = fg
  self.bg = bg
  self.decimal = nil
  self.frozen = nil
  self.mode = ""
end

function display:update()
  screen.box(self.x,self.y,self.w,self.h,self.bg)
  screen.color(self.fg,self.bg)
  local w,h = screen.font(0)
  local v = tostring(self.value)
  local e = string.find(v,"e")

  if e then
    v = string.sub(v,1,math.min(e-1,10)) .. string.sub(v,e)
  elseif v == "[inf]" or v == "[nan]" then
    v,self.value = 0,0
    screen.moveto(self.x+2, self.y + self.h - h)
    io.write("ERR")
  else
    if string.len(v) > self.max then
      v = string.sub(v,1,self.max)
    end
  end

  _,h = screen.font(7)
  w = screen.textsize(v)
  screen.moveto(self.x + self.w - w, self.y + (self.h - h) / 2)
  io.write(v)

  screen.font(0)
  screen.moveto(self.x+2, self.y)
  io.write(self.mode)
end

function display:setmode(m)
  self.mode = m
  self:update()
end

function display:reset()
  self.value = 0
  self.decimal = nil
  self.frozen = nil
  self:update()
end

function display:set(v)
  self.value = v
  self.decimal = nil
  self.frozen = 1
  self:update()
end

function display:get()
  return tonumber(self.value)
end

function display:isfrozen()
  return self.frozen
end

function display:insert(d)
  if self.frozen then
    if d == "." then
      self.value = "0."
      self.decimal = 1
    else
      self.value = d
      self.decimal = nil
    end
    self.frozen = nil
  else
    if self.value == 0 then
      if d == "." then
        self.value = "0."
        self.decimal = 1
      elseif d > 0 then
        self.value = d
      end
    else
      if d == "." then
        if not self.decimal and string.len(self.value) < self.max then
          self.value = self.value .. d
          self.decimal = 1
        end
      elseif string.len(self.value) < self.max then
        self.value = self.value .. d
      end
    end
  end
  self:update()
end

function display:delete()
  if self.frozen then
    self.value = 0
    self.decimal = nil
    self.frozen = nil
  else
    local i = string.len(self.value)
    if (i > 1) then
      if self.decimal then
        local d = string.sub(self.value,i,i)
        if d == "." then
          self.decimal = nil
        end
      end
      self.value = string.sub(self.value,1,i-1)
    else
      self.value = "0"
      self.decimal = nil
    end
  end
  self:update()
end

-- Calculator operators

function fadd()
  display:set(stack:pop() + display:get())
end

function fsub()
  display:set(stack:pop() - display:get())
end

function fmult()
  display:set(stack:pop() * display:get())
end

function fdiv()
  display:set(stack:pop() / display:get())
end

function fcos()
  local r = display:get()
  if mode == "DEG" then
    r = math.rad(r)
  end
  display:set(math.cos(r))
end

function facos()
  local r = math.acos(display:get())
  if mode == "DEG" then
    r = math.deg(r)
  end
  display:set(r)
end

function fsin()
  local r = display:get()
  if mode == "DEG" then
    r = math.rad(r)
  end
  display:set(math.sin(r))
end

function fasin()
  local r = math.asin(display:get())
  if mode == "DEG" then
    r = math.deg(r)
  end
  display:set(r)
end

function ftan()
  local r = display:get()
  if mode == "DEG" then
    r = math.rad(r)
  end
  display:set(math.tan(r))
end

function fatan()
  local r = math.atan(display:get())
  if mode == "DEG" then
    r = math.deg(r)
  end
  display:set(r)
end

function fatan2()
  local r = math.atan2(stack:pop(), display:get())
  if mode == "DEG" then
    r = math.deg(r)
  end
  display:set(r)
end

function fmode()
  if mode == "RAD" then
    mode = "DEG"
  else
    mode = "RAD"
  end
  display:setmode(mode)
end

function finv()
  display:set(1 / display:get())
end

function fln()
  display:set(math.log(display:get()))
end

function fsqrt()
  display:set(math.sqrt(display:get()))
end

function fpow()
  display:set(math.pow(stack:pop(), display:get()))
end

function fexp()
  display:set(math.exp(display:get()))
end

function frand()
  stack:push(display:get())
  display:set(math.random())
end

function fsign()
  if display:get() ~= 0 then
    display:set(-display:get())
  end
end

function back()
  display:delete()
end

function enter()
  stack:push(display:get())
  display:reset()
end

function clear()
  display:reset()
end

function sto()
  local index,value = display:get(),stack:pop()
  display:set(value)
  memory[index] = value
end

function rcl()
  local index = display:get()
  local value = memory[index]
  if not value then
    value = 0
  end
  display:set(value)
end

function init()
  local l3dinit = loadlib("3dlib2","init")
  if not l3dinit then
    gui.alert("This app requires 3dlib2.prc")
    os.exit()
  end
  l3dinit()
  math.randomseed(os.date("%H%M%S"))
  local black = screen.rgb(0,0,0)
  white = screen.rgb(255,255,255)
  screen.color(black,screen.rgb(192,192,192))
  screen.clear()
  local width,height = screen.mode()
  local displayHeight = (3 * height) / 20
  mode = "RAD"
  display:init(2,2,width-4,displayHeight-4,black,screen.rgb(96,96,0))
  display:setmode(mode)
  local keyboardHeight = height - displayHeight
  local rows,columns = 6,6
  local bHeight,bWidth = keyboardHeight / rows,width / columns
  local x,y = 0,displayHeight

  f = {}
  memory = {}
  orange = screen.rgb(255,128,0)
  green  = screen.rgb(0,128,0)
  blue   = screen.rgb(0,0,255)
  red    = screen.rgb(255,0,0)
  screen.font(0)

  screen.color(white,green)
  x,y = 0,displayHeight
  screen.moveto(x+2,y+2) f[gui.button3d("ln",   bWidth-4,bHeight-4)] = fln   y = y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d("exp",  bWidth-4,bHeight-4)] = fexp  y = y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d("1/x",  bWidth-4,bHeight-4)] = finv  y = y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d("sqrt", bWidth-4,bHeight-4)] = fsqrt y = y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d("y^x",  bWidth-4,bHeight-4)] = fpow  y = y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d("rnd",  bWidth-4,bHeight-4)] = frand

  screen.color(white,orange)
  x,y = bWidth,displayHeight
  screen.moveto(x+2,y+2) f[gui.button3d("cos", bWidth-4,bHeight-4)] = fcos  x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("sin", bWidth-4,bHeight-4)] = fsin  x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("tan", bWidth-4,bHeight-4)] = ftan  x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("mod", bWidth-4,bHeight-4)] = fmode x = x + bWidth
  screen.color(white,red)
  screen.moveto(x+2,y+2) f[gui.button3d("sto", bWidth-4,bHeight-4)] = sto
  screen.color(white,orange)
  x,y = bWidth,y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d("acos", bWidth-4,bHeight-4)] = facos  x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("asin", bWidth-4,bHeight-4)] = fasin  x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("atan", bWidth-4,bHeight-4)] = fatan  x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("atn2", bWidth-4,bHeight-4)] = fatan2 x = x + bWidth
  screen.color(white,red)
  screen.moveto(x+2,y+2) f[gui.button3d("rcl",  bWidth-4,bHeight-4)] = rcl

  screen.color(white,blue)
  x,y = bWidth,y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d(7,   bWidth-4,bHeight-4)] = 7    x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d(8,   bWidth-4,bHeight-4)] = 8    x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d(9,   bWidth-4,bHeight-4)] = 9    x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("/", bWidth-4,bHeight-4)] = fdiv x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("<", bWidth-4,bHeight-4)] = back

  x,y = bWidth,y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d(4,   bWidth-4,bHeight-4)] = 4     x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d(5,   bWidth-4,bHeight-4)] = 5     x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d(6,   bWidth-4,bHeight-4)] = 6     x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("*", bWidth-4,bHeight-4)] = fmult x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("C", bWidth-4,bHeight-4)] = clear

  x,y = bWidth,y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d(1,     bWidth-4,bHeight-4)] = 1       x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d(2,     bWidth-4,bHeight-4)] = 2       x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d(3,     bWidth-4,bHeight-4)] = 3       x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("-",   bWidth-4,bHeight-4)] = fsub    x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("Ent", bWidth-4,bHeight*2-4)] = enter

  x,y = bWidth,y + bHeight
  screen.moveto(x+2,y+2) f[gui.button3d(0,     bWidth-4,bHeight-4)] = 0     x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("+/-", bWidth-4,bHeight-4)] = fsign x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d(".",   bWidth-4,bHeight-4)] = "."   x = x + bWidth
  screen.moveto(x+2,y+2) f[gui.button3d("+",   bWidth-4,bHeight-4)] = fadd

  display:reset()
end

function eventloop()
  while true do
    local e,id = gui.event()
    if e == ctlSelect then
      if type(f[id]) == "function" then
        f[id]()
      else
        if display:isfrozen() then
          stack:push(display:get())
        end
        display:insert(f[id])
      end
    elseif e == appStop then
      break
    end
  end
end

init()
eventloop()
