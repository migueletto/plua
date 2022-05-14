-- Aquario.lua
-- Copyright (c) Marcio M. Andrade


-- Tank class definition

Tank = {}

function Tank:init(id)
  -- The background is composed of 4 bitmaps of same size in a 2x2 matrix.
  -- This is necessary because in high density mode a 320x320 bitmap
  -- does not fit in a single resource (PalmOS limitation).
  -- The id parameter is the bitmap resource id of the upper left quarter.

  local bmp, dx, dy

  -- Load the upper left quarter and get its dimensions
  bmp = resource.open("Tbmp", id)
  _, dx, dy = resource.size(bmp)

  -- Create the offscreen buffer and selects it for writing
  self.width = dx * 2
  self.height = dy * 2
  self.buffer = buffer.new(self.width, self.height)
  buffer.use(self.buffer)

  -- Draw the upper left quarter
  screen.moveto(0, 0)
  resource.draw(bmp)
  resource.close(bmp)

  -- Load the upper right quarter and draw it
  screen.moveto(dx, 0)
  bmp = resource.open("Tbmp", id+1)
  resource.draw(bmp)
  resource.close(bmp)

  -- Load the bottom left quarter and draw it
  screen.moveto(0, dy)
  bmp = resource.open("Tbmp", id+2)
  resource.draw(bmp)
  resource.close(bmp)

  -- Load the bottom right quarter and draw it
  screen.moveto(dx, dy)
  bmp = resource.open("Tbmp", id+3)
  resource.draw(bmp)
  resource.close(bmp)

  -- Select the screen for writing and clear it
  buffer.use()
  screen.clear()

  -- Create the sprite background centralized on the screen
  local width,height = screen.mode()
  sprite.init(self.buffer, (width - self.width)/ 2, (height - self.height) / 2)

  -- The fish tank is initialy empty
  self.inhabitants = {}
end

function Tank:add(fish)
  -- Add one fish to the tank
  table.insert(self.inhabitants, fish)

  fish.index = table.getn(self.inhabitants)
  fish.tank = self
  fish.x = -fish.width
  fish.y = math.random(self.height - fish.height)

  -- Create the sprite
  sprite.add(fish.index, fish)
end

function Tank.animate_item(i, fish)
  -- Helper function to iterate the inhabitants table (animate)
  fish:move()
end

function Tank:animate()
  -- Iterate the inhabitants table and move each fish
  table.foreachi(self.inhabitants, self.animate_item)

  -- Update the sprite engine
  sprite.update()
end

function Tank.destroy_item(i, fish)
  -- Helper function to iterate the inhabitants table (destroy)
  sprite.remove(fish.index)
  fish:destroy()
end

function Tank:finish()
  -- Finish the sprite engine and free the offscreen buffer
  sprite.finish()
  buffer.free(self.buffer)

  -- Iterate the inhabitants table and destroy each fish
  table.foreachi(self.inhabitants, self.destroy_item)
end


-- Fish class definition

Fish = {}

function Fish.reverse(fish)
  -- Reverse the direction of a fish by swapping its bitmap.

  fish.right = not fish.right

  if fish.right then
    fish.bitmap = fish.rightSide
  else
    fish.bitmap = fish.leftSide
  end
end

function Fish.collision(fish1,fish2)
  -- The sprite collision function. If two fishes collide and are moving
  -- in opposite directions, reverse both.

  if fish1.right ~= fish2.right then
    local now = os.time()

    -- This check avoids a fast reverse cycle between two fishes

    if (now - fish1.lasthit) > 1 then
      fish1:reverse()
      fish1.lasthit = now
    end

    if (now - fish2.lasthit) > 1 then
      fish2:reverse()
      fish2.lasthit = now
    end
  end
end

function Fish.move(fish)
  -- Move a fish by updating its horizontal position according to its speed

  if fish.right then
    fish.x = fish.x + fish.speed
  else
    fish.x = fish.x - fish.speed
  end

  -- If the fish moved out of the screen, reverse its direction
  -- and set a new vertical position.

  if fish.x < -fish.width or fish.x > fish.tank.width then
    fish:reverse()
    fish.lasthit = 0
    fish.y = math.random(fish.tank.height - fish.height)
  end
end

function Fish.destroy(fish)
  -- Close the bitmap resources allocated to the fish
  resource.close(fish.rightSide)
  resource.close(fish.leftSide)
end

function Fish:new(id)
  -- Ceate a new Fish object.
  -- A fish is a Lua table with various fields and methods.

  -- Loads two bitmaps. The parameter id is the bitmap pointing right,
  -- and id+1000 is the bitmap pointing left.
  local b1 = resource.open("Tbmp", id)
  local b2 = resource.open("Tbmp", id + 1000)
  local size, dx, dy = resource.size(b1)

  local fish = {
    -- These fields are required by the sprite engine
    bitmap = b1,
    active = true,
    x = 0,
    y = 0,

    -- These fields are not part of the sprite engine, but are used
    -- by the Fish class
    right = true,
    rightSide = b1,
    leftSide = b2,
    width = dx,
    height = dy,
    speed = math.random(4),
    lasthit = 0
  }

  -- Access the methods in the Fish class by defining a meta table
  setmetatable(fish, { __index = self })

  return fish
end



-- Fish sub-classes definitions

Angel = {}

function Angel:new()
  return Fish:new(1001)
end

Flame = {}

function Flame:new()
  return Fish:new(1002)
end

Pooler = {}

function Pooler:new()
  return Fish:new(1003)
end

Copper = {}

function Copper:new()
  return Fish:new(1004)
end



-- The main code

local start = os.clock()
local frames = 0
local last = start

function Animate()
  local now = os.clock()
  if now - last >= 0.05 then
    Tank:animate()
    last = now
    frames = frames + 1
    local fps = frames / (now - start)

    -- show frames per socond
    screen.moveto(0,0)
    print(string.format("%05.2f",fps))
  end
end

function MainForm()
  math.randomseed(os.date("%S"))

  Tank:init(3001)

  Tank:add(Angel:new())
  Tank:add(Flame:new())
  Tank:add(Pooler:new())
  Tank:add(Copper:new())

  gui.sethandler(nilEvent, Animate)
  gui.sethandler(penDown, Animate)
  gui.sethandler(penMove, Animate)
  gui.sethandler(keyDown, Animate)

  gui.main(50)
  Tank:finish()
end

MainForm()
