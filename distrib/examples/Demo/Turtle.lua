-- Turtle.lua
-- Copyright (c) Marcio M. Andrade


-- Turtle graphics demo

function hue(angle)
  local red = 127 * (1 + math.sin(angle))
  local green = 127 * (1 + math.sin(math.rad(120) + angle))
  local blue = 127 * (1 + math.sin(math.rad(240) + angle))
  return screen.rgb(red,green,blue)
end

function square(side, seed)
  while side >= 4 do
    screen.jump(-side/2)
    screen.turn(-math.rad(90))
    screen.walk(-side/2)
    screen.color(hue(seed*side))
    for i = 1,4 do
      screen.walk(side)
      screen.turn(math.rad(90))
    end
    screen.jump(side/2)
    screen.turn(math.rad(90))
    screen.jump(side/2)
    screen.turn(math.rad(4))
    side = side - 4
  end
end

function MainForm()
  screen.clear(screen.rgb(0,0,0))
  local width,height = screen.mode()
  local seed = 1
  gui.sethandler(nilEvent,
    function()
      screen.moveto(width/2,height/2)
      screen.heading(-math.rad(6))
      square(width, seed)
      seed = seed * 1.02
    end
  )
end

MainForm()
gui.main(50)
