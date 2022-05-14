-- LuaPad.lua

function MemoInit()
  local f, index

  memoCatName = {}
  memoCatIndex = {}
  memoNcats = 0
  currentCat = nil

  f = io.open("db:/MemoDB", "r")

  if not f then
    return
  end

  for index = 1,15 do
    local s = f:getdbcat(index-1)
    if s and s ~= "" then
      memoNcats = memoNcats + 1
      memoCatName[memoNcats] = s
      memoCatIndex[memoNcats] = index-1
    end
  end

  f:close()

  currentCat = 1
  current = 1
end

function MemoRefresh()
  local f, n, index

  memoName = {}
  memoIndex = {}
  memoNrecs = 0

  f, n = io.open("db:/MemoDB", "r")

  if not f then
    return
  end

  for index = 1, n do
    if f:getreccat(index-1) == memoCatIndex[currentCat] then
      if f:openrec(index-1) then
        local title = f:read("*l")
        memoNrecs = memoNrecs + 1
        memoName[memoNrecs] = title
        memoIndex[memoNrecs] = index-1
      end
    end
  end

  f:close()
end

function MemoList()
  gui.destroy()
  screen.clear()
  gui.title("Memo")

  MemoRefresh()

  local list = gui.list(11, 31, memoName, current)
  gui.nl()

  local view = gui.button("View")

  local width = screen.mode()
  screen.moveto(width/2, 0)
  local cat = gui.popup(memoCatName, currentCat)

  while true do
    local ev, id, index = gui.event()
    if ev == ctlSelect and id == view then
      if memoNrecs > 0 then
        return true
      end
    elseif ev == lstSelect and id == list then
      current = index
    elseif ev == popSelect and id == cat then
      currentCat = index
      MemoRefresh()
      current = 1
      gui.settext(list, memoName)
      if memoNrecs > 0 then
        gui.setstate(list, current)
      end
    elseif ev == appStop then
      return false
    end
  end
end

function MemoView()
  gui.destroy()
  screen.clear()

  local f = io.open("db:/MemoDB", "r")

  local s = ""
  if f:openrec(memoIndex[current]) then
    s = f:read("*a")
  end
  f:close()

  gui.title("Memo " .. current .. " of " .. memoNrecs)
  gui.field(11, 30, 4096, s, nil, nil)
  gui.nl()

  local done = gui.button("Done")

  while true do
    local ev, id = gui.event()
    if ev == ctlSelect and id == done then
      return true
    elseif ev == appStop then
      return false
    end
  end
end

MemoInit()

while true do
  if not MemoList() then
    break
  end

  if not MemoView() then
    break
  end
end
