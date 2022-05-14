-- Sound.lua
-- Copyright (c) Marcio M. Andrade


function MainForm()
  gui.title("Sound")

  gui.control {
    type = "label",
    text = "Volume",
    font = 1
  }

  volume = gui.control {
    type = "slider",
    width = screen.mode() - screen.pos() - 4,
    state = 8,
    limit = 17
  }

  gui.nl()

  gui.control {
    type = "label",
    text = "System MIDI",
    font = 1
  }

  local systemMidiDb = "db:/System MIDI Sounds"
  local f,n = io.open(systemMidiDb)
  local recNames = {}
  local i
  for i = 1,n do
    f:openrec(i-1)
    local header = f:read(6)
    local t = bin.unpack("B", header, 4)
    local name = f:read(t[1] - 7)
    recNames[i] = name
  end
  f:close()

  local systemMidis = gui.control {
    type = "popup",
    list = recNames
  }

  gui.control {
    type = "button",
    text = "Play",
    handler = function ()
      local id = gui.getstate(systemMidis)-1
      local vol = (gui.getstate(volume)-1)*4
      local r,msg = sound.midi(systemMidiDb.."/"..id, vol)
      if r == nil then
        gui.alert(msg)
      end
    end
  }

  gui.nl()

  gui.control {
    type = "label",
    text = "Custom MIDI",
    font = 1
  }

  local midiNameIds = resource.list("MIDn", "Sound")
  local midiNames = {}
  n = table.getn(midiNameIds)
  for i = 1,n do
    local r = resource.open("MIDn", midiNameIds[i], "Sound")
    midiNames[i] = resource.get(r)
    resource.close(r)
  end

  local customMidis = gui.control {
    type = "popup",
    list = midiNames
  }

  gui.control {
    type = "button",
    text = "Play",
    handler = function ()
      local id = gui.getstate(customMidis)
      local vol = (gui.getstate(volume)-1)*4
      local r,msg = sound.midi("rsrc:/MIDf/"..id, vol)
      if r == nil then
        gui.alert(msg)
      end
    end
  }

  gui.nl()

  gui.control {
    type = "label",
    text = "WAV",
    font = 1
  }

  local wavNameIds = resource.list("WAVn", "Sound")
  local wavNames = {}
  n = table.getn(wavNameIds)
  for i = 1,n do
    local r = resource.open("WAVn", wavNameIds[i], "Sound")
    wavNames[i] = resource.get(r)
    resource.close(r)
  end

  local waves = gui.control {
    type = "popup",
    list = wavNames
  }

  gui.control {
    type = "button",
    text = "Play",
    handler = function ()
      local id = gui.getstate(waves)
      local vol = (gui.getstate(volume)-1)*4
      local r,msg = sound.play("rsrc:/WAVf/"..id, 1, vol)
      if r == nil then
        gui.alert(msg)
      end
    end
  }

  gui.control {
    type = "button",
    text = "Stop",
    handler = function ()
      sound.stop(1)
    end
  }

  gui.nl()
end

MainForm()
gui.main()
