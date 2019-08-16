function generateVersionFile()
	local version = getSingleLineOutput('git describe --exact-match')
	local branch = getSingleLineOutput('git symbolic-ref --short -q HEAD')
	local commit = getSingleLineOutput('git rev-parse --short=7 HEAD')
	local date = os.date("!%Y-%m-%d %H:%M:%S")
	local releaseBuild = false

	if os.getenv("CI") ~= nil then 
		releaseBuild = true
	end
	if os.getenv("APPVEYOR") == "True" then
		branch = os.getenv("APPVEYOR_REPO_BRANCH")
	end

	local versionString = ''

	if version == '' then
		versionString = string.format("%s-%s (%s)", branch, commit, date)
	else
		versionString = string.format("v%s", version)
	end

	versionString = versionString .. " \" BUILD_ARCH \""

	f = io.open('src/version.h', 'w')
	f:write('#pragma once\n')
	f:write(string.format('#ifndef BUILD_ARCH\n'))
	f:write(string.format('    #define BUILD_ARCH "UNKNOWN-ARCH"\n'))
	f:write(string.format('#endif\n'))
	f:write(string.format('#define BUILD_VERSION "%s"\n', version))
	f:write(string.format('#define BUILD_BRANCH "%s"\n', branch))
	f:write(string.format('#define BUILD_COMMIT "%s"\n', commit))
	f:write(string.format('#define BUILD_DATE "%s"\n', date))
	f:write(string.format('#define BUILD_STRING "%s"\n', versionString))
	if releaseBuild == true then 
		f:write('#define BUILD_IS_RELEASE true\n')
	end
	f:close()
end

function getSingleLineOutput(command)
	return string.gsub(getOutput(command), '\n', '')
end

function getOutput(command)
	local nullDevice = ''
	if package.config:sub(1,1) == '\\' then
		nullDevice = 'NUL'
	else
		nullDevice = '/dev/null'
	end

	local file = io.popen(command .. ' 2> ' .. nullDevice, 'r')
	local output = file:read('*all')
	file:close()

	return output
end

