-- Demo1.lua
-- Copyright (c) Marcio M. Andrade


-- GUI controls and events demo

local labels = {
  [menuSelect] = "mnu",
  [ctlSelect]  = "ctl",
  [ctlRepeat]  = "rpt",
  [lstSelect]  = "lst",
  [popSelect]  = "pop",
  [nilEvent]   = "nil"
}

function PutLabel(label)
  local w = screen.mode()
  local p = screen.textsize(label)
  gui.control{type="label", text=label, x=w/2-p}
  screen.moveto(w/2 + w/16)
end

function LogEvent(e, id, arg)
  local w = screen.mode()
  local _,h = screen.font(0)
  screen.box(w/2, 0, w/2, h, screen.rgb(255, 255, 255))
  screen.moveto(w/2, 0)
  if arg then
    print(string.format("%.2f: %s %d %d", os.clock(), labels[e], id, arg))
  elseif id then
    print(string.format("%.2f: %s %d", os.clock(), labels[e], id))
  else
    print(string.format("%.2f: %s", os.clock(), labels[e]))
  end
end

function MainForm()
  gui.title("Demo 1")

  gui.menu{"R:Red", "B:Blue", "-", "G:Green", "Y:Yellow"}
  gui.sethandler(menuSelect, LogEvent)

  PutLabel("Buttons:")
  gui.control{type="button", text="Open", handler=LogEvent}
  gui.control{type="button", bitmap=1001, handler=LogEvent}
  gui.nl()

  PutLabel("Push buttons:")
  gui.control{type="pbutton", text="Left", group=1, state=1, handler=LogEvent}
  gui.control{type="pbutton", text="Right", group=1, handler=LogEvent}
  gui.nl()

  PutLabel("Repeating button:")
  gui.control{type="rbutton", text="Add", handler=LogEvent}
  gui.nl()

  PutLabel("Checkbox:")
  gui.control{type="checkbox", text="Mark", handler=LogEvent}
  gui.nl()

  PutLabel("Selector trigger:")
  gui.control{type="selector", text="Select", handler=LogEvent}
  gui.nl()

  PutLabel("Slider:")
  local w = screen.mode() - screen.pos() - 4
  gui.control{type="slider", width=w, limit=5, state=3, handler=LogEvent}
  gui.nl()

  PutLabel("Popup:")
  gui.control{type="popup", list={"One", "Two", "Three", "Four"}, selected=2, handler=LogEvent}
  gui.nl()

  gui.control{type="label", text="List:"}
  gui.control{type="list", lines=2, columns=10, list={"One", "Two", "Three", "Four"}, handler=LogEvent}

  gui.control{type="label", text="Field:"}
  gui.control{type="field", lines=2, columns=8, text="One\nTwo\nThree\nFour"}

  gui.sethandler(nilEvent, LogEvent)
end

MainForm()
gui.main(5000)
