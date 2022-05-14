-- ScreenTest.lua

init = loadlib("ScreenLib", "init")
init()

t = {screen.test1,screen.test2,screen.test3,screen.test4,screen.test5,
     screen.test6,screen.test7,screen.test8,screen.test9}

for i,f in t do
  print("Test "..i..": ",f())
end

gui.event()
