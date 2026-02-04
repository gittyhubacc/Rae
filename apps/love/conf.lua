local settings = {}

settings.pixel = { w = 32, h = 32 }
settings.res = { w = 12, h = 12 }


function love.conf(t)
  t.window.title = 'img processing'
  t.window.width = settings.pixel.w * settings.res.w
  t.window.height = settings.pixel.h * settings.res.h
end

return settings
