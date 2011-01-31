const char default_vs[] = 
  "varying vec3 eye,dir;"
  "uniform float fov_x,fov_y;"
  "uniform float x_offset,x_scale,y_offset,y_scale;"
  "float fov2scale(float fov){return tan(radians(fov/2.0));}"
  "void main(){"
    "gl_Position=gl_Vertex;"
    "eye=vec3(gl_ModelViewMatrix[3]);"
    "dir=vec3(gl_ModelViewMatrix*"
      "vec4("
      "fov2scale(fov_x)/x_scale*(x_scale*gl_Vertex.x+x_offset),"
      "fov2scale(fov_y)*(y_scale*gl_Vertex.y+y_offset),"
      "1,"
      "0));"
  "}";

const char default_fs[] = 
  "#define P0 p0\n"
  "#define SCALE par[0].y\n"
  "#define MINRAD2 par[0].x\n"
  "#define DIST_MULTIPLIER 1.0\n"
  "#define MAX_DIST 4.0\n"
  "varying vec3 eye,dir;"
  "uniform vec3 par[10];"
  "uniform float"
   " min_dist,"
    "ao_eps,"
    "ao_strength,"
    "glow_strength,"
    "dist_to_color;"
  "uniform int iters,"
    "color_iters,"
    "max_steps;"
  "vec3 backgroundColor=vec3(0.07,0.06,0.16),"
    "surfaceColor1=vec3(0.95,0.64,0.1),"
    "surfaceColor2=vec3(0.89,0.95,0.75),"
    "surfaceColor3=vec3(0.55,0.06,0.03),"
    "specularColor=vec3(1.0,0.8,0.4),"
    "glowColor=vec3(0.03,0.4,0.4),"
    "aoColor=vec3(0,0,0);"
  "float minRad2=clamp(MINRAD2,1.0e-9,1.0);"
  "vec4 scale=vec4(SCALE,SCALE,SCALE,abs(SCALE))/minRad2;"
  "float absScalem1=abs(SCALE-1.0);"
  "float AbsScaleRaisedTo1mIters=pow(abs(SCALE),float(1-iters));"
  "float d(vec3 pos){"
    "vec4 p=vec4(pos,1),p0=p;"
    "for(int i=0;i<iters;i++){"
      "p.xyz=clamp(p.xyz,-1.0,1.0)*2.0-p.xyz;"
      "float r2=dot(p.xyz,p.xyz);"
      "p*=clamp(max(minRad2/r2,minRad2),0.0,1.0);"
      "p=p*scale+P0;"
    "}"
    "return((length(p.xyz)-absScalem1)/p.w-AbsScaleRaisedTo1mIters)*DIST_MULTIPLIER;"
  "}"
  "vec3 color(vec3 pos){"
    "vec3 p=pos,p0=p;"
    "float trap=1.0;"
    "for(int i=0;i<color_iters;i++){"
      "p.xyz=clamp(p.xyz,-1.0,1.0)*2.0-p.xyz;"
      "float r2=dot(p.xyz,p.xyz);"
      "p*=clamp(max(minRad2/r2,minRad2),0.0,1.0);"
      "p=p*scale.xyz+P0.xyz;"
      "trap=min(trap,r2);"
    "}"
    "vec2 c=clamp(vec2(0.33*log(dot(p,p))-1.0,sqrt(trap)),0.0,1.0);"
    "return mix(mix(surfaceColor1,surfaceColor2,c.y),surfaceColor3,c.x);"
  "}"
  "float normal_eps=0.00001;"
  "vec3 normal(vec3 pos,float d_pos){"
    "vec4 Eps=vec4(0,normal_eps,2.0*normal_eps,3.0*normal_eps);"
    "return normalize(vec3("
      "-d(pos-Eps.yxx)+d(pos+Eps.yxx),"
      "-d(pos-Eps.xyx)+d(pos+Eps.xyx),"
      "-d(pos-Eps.xxy)+d(pos+Eps.xxy)"
      "));"
  "}"
  "vec3 blinn_phong(vec3 normal,vec3 view,vec3 light,vec3 diffuseColor){"
    "vec3 halfLV=normalize(light+view);"
    "float spe=pow(max(dot(normal,halfLV),0.0),32.0);"
    "float dif=dot(normal,light)*0.5+0.75;"
    "return dif*diffuseColor+spe*specularColor;"
  "}"
  "float ambient_occlusion(vec3 p,vec3 n){"
    "float ao=1.0,w=ao_strength/ao_eps;"
    "float dist=2.0*ao_eps;"
    "for(int i=0;i<5;i++){"
      "float D=d(p+n*dist);"
      "ao-=(dist-D)*w;"
      "w*=0.5;"
      "dist=dist*2.0-ao_eps;"
    "}"
    "return clamp(ao,0.0,1.0);"
  "}"
  "void main(){"
    "vec3 p=eye,dp=normalize(dir);"
    "float totalD=0.0,D=3.4e38,extraD=0.0,lastD;"
    "int steps;"
    "for(steps=0;steps<max_steps;steps++){"
      "lastD=D;"
      "D=d(p+totalD*dp);"
      "if(extraD>0.0&&D<extraD){"
        "totalD-=extraD;"
        "extraD=0.0;"
        "D=3.4e38;"
        "steps--;"
        "continue;"
      "}"
      "if(D<min_dist||D>MAX_DIST)break;"
      "totalD+=D;"
      "totalD+=extraD=0.096*D*(D+extraD)/lastD;"
    "}"
    "p+=totalD*dp;"
    "vec3 col=backgroundColor;"
    "if(D<MAX_DIST){"
      "vec3 n=normal(p,D);"
      "col=color(p);"
      "col=blinn_phong(n,-dp,normalize(eye+vec3(0,1,0)+dp),col);"
      "col=mix(aoColor,col,ambient_occlusion(p,n));"
      "if(D>min_dist){"
        "col=mix(col,backgroundColor,clamp(log(D/min_dist)*dist_to_color,0.0,1.0));"
      "}"
    "}"
    "col=mix(col,glowColor,float(steps)/float(max_steps)*glow_strength);"
    "float zFar=5.0;"
    "float zNear=0.0001;"
    "float a=zFar/(zFar-zNear);"
    "float b=zFar*zNear/(zNear-zFar);"
    "float depth=(a+b/clamp(totalD/length(dir), zNear, zFar));"
    "gl_FragDepth=depth;"
    "gl_FragColor=vec4(col,depth);"
  "}";

