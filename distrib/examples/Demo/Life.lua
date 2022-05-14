-- Life.lua
-- The original program is distributed with Lua

-- original by Dave Bollinger <DBollinger@compuserve.com> posted to lua-l
-- modified to use ANSI terminal escape sequences
-- modified to use for instead of while
-- ported to Plua 2 by Marcio Migueletto de Andrade

ALIVE = screen.rgb(0,0,255)
DEAD = screen.rgb(255,255,255)

function ARRAY2D(w,h)
  local t = {w=w,h=h}
  for y=1,h do
    t[y] = {}
    for x=1,w do
      t[y][x]=0
    end
  end
  return t
end

_CELLS = {}

-- give birth to a "shape" within the cell array
function _CELLS:spawn(shape,left,top)
  local ty=0
  for y=0,shape.h-1 do
    for x=0,shape.w-1 do
      self[top+y][left+x] = shape[ty+x+1]
    end
    ty=ty+shape.w
  end
end

-- run the CA and produce the next generation
function _CELLS:evolve(next)
  local ym1,y,yp1,yi=self.h-1,self.h,1,self.h
  while yi > 0 do
    local xm1,x,xp1,xi=self.w-1,self.w,1,self.w
    while xi > 0 do
      local sum = self[ym1][xm1] + self[ym1][x] + self[ym1][xp1] +
                  self[y][xm1] + self[y][xp1] +
                  self[yp1][xm1] + self[yp1][x] + self[yp1][xp1]
      next[y][x] = ((sum==2) and self[y][x]) or ((sum==3) and 1) or 0
      xm1,x,xp1,xi = x,xp1,xp1+1,xi-1
    end
    ym1,y,yp1,yi = y,yp1,yp1+1,yi-1
  end
end

-- output the array to screen
function _CELLS:draw(buf,x0,y0,dx,dy)
  buffer.use(buf)
  local sx,sy = 0,0
  for y=1,self.h do
    for x=1,self.w do
      screen.box(sx,sy,dx,dy,(((self[y][x]>0) and ALIVE) or DEAD))
      sx=sx+dx
    end
    sx=0
    sy=sy+dy
  end
  buffer.use()
  buffer.put(buf,x0,y0)
end

-- constructor
function CELLS(w,h)
  local c = ARRAY2D(w,h)
  c.spawn = _CELLS.spawn
  c.evolve = _CELLS.evolve
  c.draw = _CELLS.draw
  return c
end

--
-- shapes suitable for use with spawn() above
--
HEART = { 1,0,1,1,0,1,1,1,1; w=3,h=3 }
GLIDER = { 0,0,1,1,0,1,0,1,1; w=3,h=3 }
EXPLODE = { 0,1,0,1,1,1,1,0,1,0,1,0; w=3,h=4 }
FISH = { 0,1,1,1,1,1,0,0,0,1,0,0,0,0,1,1,0,0,1,0; w=5,h=4 }
BUTTERFLY = { 1,0,0,0,1,0,1,1,1,0,1,0,0,0,1,1,0,1,0,1,1,0,0,0,1; w=5,h=5 }

-- the main routine
function LIFE(w,h)
  gui.title("Life")

  -- create two arrays
  local thisgen = CELLS(w,h)
  local nextgen = CELLS(w,h)

  -- create some life
  -- about 1000 generations of fun, then a glider steady-state
  thisgen:spawn(GLIDER,5,4)
  thisgen:spawn(EXPLODE,25,10)
  thisgen:spawn(FISH,4,12)

  local fx,fy = screen.font(0)
  local sx,sy = screen.mode()
  local dx,dy = fx/2,fy/2
  local tx,ty = w*dx,h*dy
  local x0,y0 = sx/2-tx/2,sy/2-ty/2
  local buf = buffer.new(tx,ty)
  screen.rect(x0,y0,tx+2,ty+2)
  local x1,y1 = x0+screen.textsize("Generation "),y0+ty+6
  screen.moveto(x0,y1)
  print("Generation ")

  local gen=1
  gui.sethandler(nilEvent,
    function()
      thisgen:evolve(nextgen)
      thisgen,nextgen = nextgen,thisgen
      thisgen:draw(buf,x0+1,y0+1,dx,dy)
      screen.moveto(x1,y1)
      print(gen)
      gen=gen+1
    end
  )
  gui.main(200)
  buffer.free(buf)
end

LIFE(40,20)
