var Mirobot = function(url){
  this.url = url;
  this.connect();
  this.cbs = {};
  this.listeners = [];
}

Mirobot.prototype = {

  connected: false,

  connect: function(){
    if(!this.connected){
      var self = this;
      this.ws = new WebSocket(this.url);
      this.ws.onmessage = function(ws_msg){self.handle_ws(ws_msg)};
      this.ws.onopen = function(){ self.setConnectedState(true)}
      this.ws.onerror = function(err){self.handleError(err)}
      this.ws.onclose = function(err){self.handleError(err)}
    }
  },

  setConnectedState: function(state){
    var self = this;
    self.connected = state;
    setTimeout(function(){
      self.broadcast(self.connected ? 'connected' : 'disconnected');
    }, 500);
    // Try to auto reconnect if disconnected
    if(state){
      if(self.reconnectTimer){
        clearInterval(self.reconnectTimer);
        self.reconnectTimer = undefined;
      }
    }else{
      if(!self.reconnectTimer){
        self.connect();
        self.reconnectTimer = setInterval(function(){
          self.connect();
        }, 1000);
      }
    }
  },
  
  broadcast: function(msg){
    for(i in this.listeners){
      this.listeners[i](msg);
    }
  },

  addListener: function(listener){
    this.listeners.push(listener);
  },

  handleError: function(err){
    if(err instanceof CloseEvent){
      this.setConnectedState(false);
    }else{
      console.log(err);
    }
  },

  move: function(direction, distance, cb){
    this.send({cmd: direction, arg: distance}, cb);
  },

  turn: function(direction, angle, cb){
    this.send({cmd: direction, arg: angle}, cb);
  },

  penup: function(cb){
    this.send({cmd: 'penup'}, cb);
  },

  pendown: function(cb){
    this.send({cmd: 'pendown'}, cb);
  },

  beep: function(duration, cb){
    this.send({cmd: 'beep', arg: duration}, cb);
  },

  collide: function(cb){
    this.send({cmd: 'collide'}, cb);
  },

  follow: function(cb){
    this.send({cmd: 'follow'}, cb);
  },

  stop: function(cb){
    var self = this;
    this.send({cmd:'stop'}, function(state, msg, recursion){
      if(state === 'complete' && !recursion){
        for(var i in self.cbs){
          self.cbs[i]('complete', undefined, true);
        }
        self.robot_state = 'idle';
        self.msg_stack = [];
        self.cbs = {};
        if(cb){ cb(state); }
      }
    });
  },
  
  pause: function(cb){
    this.send({cmd:'pause'}, cb);
  },
  
  resume: function(cb){
    this.send({cmd:'resume'}, cb);
  },
  
  ping: function(cb){
    this.send({cmd:'ping'}, cb);
  },

  version: function(cb){
    this.send({cmd:'version'}, cb);
  },

  send: function(msg, cb){
    msg.id = Math.random().toString(36).substr(2, 10)
    if(cb){
      this.cbs[msg.id] = cb;
    }
    if(msg.arg){ msg.arg = msg.arg.toString(); }
    if(['stop', 'pause', 'resume', 'ping', 'version'].indexOf(msg.cmd) >= 0){
      console.log(msg);
      this.ws.send(JSON.stringify(msg));
    }else{
      this.push_msg(msg);
    }
  },
  
  push_msg: function(msg){
    this.msg_stack.push(msg);
    this.run_stack();
  },
  
  run_stack: function(){
    if(this.robot_state === 'idle' && this.msg_stack.length > 0){
      this.robot_state = 'receiving';
      console.log(this.msg_stack[0]);
      this.ws.send(JSON.stringify(this.msg_stack[0]));
    }
  },
  
  handle_ws: function(ws_msg){
    msg = JSON.parse(ws_msg.data);
    console.log(msg);
    if(this.msg_stack.length > 0 && this.msg_stack[0].id == msg.id){
      if(msg.status === 'accepted'){
        if(this.cbs[msg.id]){
          this.cbs[msg.id]('started', msg);
        }
        this.robot_state = 'running';
      }else if(msg.status === 'complete'){
        if(this.cbs[msg.id]){
          this.cbs[msg.id]('complete', msg);
          delete this.cbs[msg.id];
        }
        this.msg_stack.shift();
        if(this.msg_stack.length === 0){
          this.broadcast('program_complete');
        }
        this.robot_state = 'idle';
        this.run_stack();
      }
    }else{
      if(this.cbs[msg.id]){
        this.cbs[msg.id]('complete', msg);
        delete this.cbs[msg.id];
      }
    } 
  },
  
  robot_state: 'idle',
  msg_stack: []
}
