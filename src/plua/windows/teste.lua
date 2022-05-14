ui.clear()
ui.title("Programa de teste")
dx,dy,depth=ui.mode()

ui.button("Teste")
ui.button("Qwerty") ui.nl()
ui.checkbox("Marcio")
ui.checkbox("Andrade") ui.nl()
ui.pbutton("Um")
ui.pbutton("dois") ui.nl()
ui.field(3,20,100,dx.."x"..dy..", "..depth.." bpp") ui.nl()
ui.list(4,10,{"Um","Dois","Tres","Quatro"})
ui.popup({"Um","Dois","Tres","Quatro"},2)
ui.slider(100,5) ui.nl()
x,y=ui.pos()
ui.circle(x+40,y+40,40,40,ui.rgb(0,0,255))
ui.disc(x+120,y+40,40,40,ui.rgb(255,0,0))
ui.fill(x+40,y+40,ui.rgb(0,255,0))

while 1 do
  e,a1,a2,a3 = ui.event()
  --if e == ui.ctlSelect then
    --ui.alert(a1)
  --end
  if e == ui.keyDown then
    print(a1)
  end
  if e == ui.nilEvent then
    print("wait")
  end
  if e == ui.appStop then
    break
  end
end
