#!/usr/bin/lua

if(arg[1]=='build') then	-- for internal usage only

    if(arg[2]==nil or arg[3]==nil) then
	os.exit(1)
    end
    
    os.execute(string.format('dpkg -b %s %s/debian-tmp.deb',arg[2],arg[3]))
    os.execute('rm -rf '..arg[2])
    os.execute(string.format('dpkg-name -o %s/debian-tmp.deb',arg[3]))
    os.exit(0)
end

if(arg[1]==nil) then
    print('USAGE: ./mkdeb.lua etch|hardy')
    os.exit(1)
end


deb=
{
    conf={}
}

dofile(string.format('conf/%s.lua',arg[1] or 'def'))

deb.source_path='.'
deb.target_path='/tmp/debroot'
deb.archives_path='.'

function deb.def(n,v)
    deb.conf[n]=v
end

function deb.get_var(n) return deb.conf[n] or '' end

function deb.top(path)
    os.execute(string.format('rm -rf %s/*',deb.target_path))
    os.execute('mkdir -p '..deb.target_path)
    deb.source_path=path
end

function deb.control(control_file)
    os.execute(string.format('mkdir -p %s/DEBIAN',deb.target_path))

    deb.pcopy(deb.source_path..'/'..control_file,deb.target_path..'/DEBIAN/control')
end

function deb.perm(file,p)
    os.execute(string.format('chmod -R %s %s/%s',p,deb.target_path,file))
end

function deb.mkdir(path)
    os.execute(string.format('mkdir -p %s/%s',deb.target_path,path))
end

function deb.copy(from,to)
    os.execute(string.format('cp -rf %s/%s %s/%s',deb.source_path,from,deb.target_path,to or ''))
end

function deb.copy_tmpl(from,to)
    deb.pcopy(deb.source_path..'/'..from,deb.target_path..'/'..to)
end

function deb.pcopy(from,to)
    local f=io.open(from,'r')
    local t=io.open(to,'w')

    if(f==nil) then return '' end
    if(t==nil) then f:close() return '' end

    for s in f:lines() do
	t:write(string.gsub(s,"%{([%w_]+)%}",deb.get_var),'\n')
    end

    f:close()
    t:close()
end


function deb.build()
    os.execute(string.format('fakeroot -- lua %s build %s %s',arg[0],deb.target_path,deb.archives_path))
end


-- make debs
dofile('conf/packages.lua')
