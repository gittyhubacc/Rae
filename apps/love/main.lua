local conf = require 'conf'
local env = require 'env'

local game = { colors = { } }
game.colors[env.white] = {1, 1, 1}
game.colors[env.black] = {0, 0, 0}

function love.load()
	-- draw cross
	game.blank = env.new_image()
	game.cross = env.new_image()
	local bumpx = conf.res.w % 2 == 0 and 0.5 or 0
	local bumpy = conf.res.w % 2 == 0 and 0.5 or 0
	for y = 0, conf.res.h - 1 do
		for x = 0, conf.res.w - 1 do
			if
				math.abs(((conf.res.w / 2) - bumpx) - x) <= 0.5
				or math.abs(((conf.res.h / 2) - bumpy) - y) <= 0.5
			then
				env.set_pixel(game.cross, x, y, env.black)
			end
		end
	end
	game.filtered = env.main_filter(game.cross)
end

function love.draw()
	local subject = game.filtered
	for y = 0, conf.res.h - 1 do
		for x = 0, conf.res.w - 1 do
			local ww = conf.pixel.w
			local hh = conf.pixel.h
			local xx = x * conf.pixel.w
			local yy = y * conf.pixel.h
			local pixel = env.get_pixel(subject, x, y)
			love.graphics.setColor(game.colors[pixel])
			love.graphics.rectangle('fill', xx, yy, ww, hh)
		end
	end
end
