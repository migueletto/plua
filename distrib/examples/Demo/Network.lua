-- Network.lua
-- Copyright (c) Marcio M. Andrade

function Field(name, nl, nc, max)
  gui.control {
    type = "label",
    text = name,
    font = 1
  }

  if nl > 1 then
    gui.nl()
  end

  local f = gui.control {
    type = "field",
    lines = nl,
    columns = nc,
    limit = max or nl*nc
  }

  return f
end

function MainForm()
  gui.title("Network")

  local protocol = "udp:/"
  local host = Field("Host:", 1, 20, 32) gui.nl()
  local port = Field("Port:", 1, 5)
  local timeouts = "/5/10/0" -- dnsTimeout: 5s, connectTimeout: 10s, linger: on, 0s
  local sock = nil

  gui.control {
    type = "pbutton",
    text = "UDP",
    group = 1,
    state = 1,
    handler = function ()
      protocol = "udp:/"
    end
  }

  gui.control {
    type = "pbutton",
    text = "TCP",
    group = 1,
    handler = function ()
      protocol = "tcp:/"
    end
  }

  gui.nl() gui.nl()

  gui.control {
    type = "button",
    text = "Connect",
    handler = function ()
      -- if socket is open, close it before connecting again
      if sock ~= nil then
        sock:close()
      end
      -- connect to the remote host
      sock,error = io.open(protocol..gui.gettext(host)..":"..gui.gettext(port)..timeouts)
      if sock == nil then
        gui.alert(error)
      else
        -- send a single newline character
        sock:write("\n")
      end
    end
  }

  gui.control {
    type = "button",
    text = "Abort",
    handler = function ()
      if sock ~= nil then
        sock:close()
        sock = nil
      end
    end
  }

  gui.nl() gui.nl()
  local reply = Field("Reply", 4, 30, 256)

  gui.sethandler(ioPending,
    function ()
      -- read up to 256 bytes from the remote host and close the connection
      local text = sock:read(256)
      gui.settext(reply, text)
      sock:close()
      sock = nil
    end
  )
end

MainForm()
gui.main()
