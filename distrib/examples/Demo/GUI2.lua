-- Demo2.lua
-- Copyright (c) Marcio M. Andrade


-- GUI dialogs demo

function Dialog()
  local w,h = screen.mode()
  gui.dialog(w/4, h/4, w/2, h/2, "Dialog")
  gui.control{type="field", lines=4, columns=14, text="This is a custom dialog"}
  gui.nl()
  gui.tab()
  gui.control{type="button", text="Ok", handler=function() gui.destroy() end}
end

function PutButton(label, h)
  local w = screen.mode()
  local p = screen.textsize(label) + screen.textsize("a")
  gui.control{type="button", text=label, x=(w-p)/2, width=p, handler=h}
  gui.nl()
end

function MainForm()
  gui.title("Demo 2")
  gui.nl()

  PutButton("Custom Dialog", Dialog)
  PutButton("Alert Dialog", function() gui.alert("This is an alert") end)
  PutButton("Confirmation Dialog", function() gui.confirm("Do you agree ?") end)
  PutButton("Input Dialog", function() gui.input("Enter text") end)
  PutButton("Select Color Dialog", function() gui.selectcolor("Select a color", screen.rgb(255,128,64)) end)
  PutButton("Select Date Dialog", function() gui.selectdate("Select a date") end)
  PutButton("Select Time Dialog", function() gui.selecttime("Select a time") end)
end

MainForm()
gui.main()
