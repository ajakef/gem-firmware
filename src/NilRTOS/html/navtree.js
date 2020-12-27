var NAVTREE =
[
  [ "Nil RTOS", "index.html", [
    [ "Nil RTOS", "index.html", [
      [ "Examples", "examples.html", null ],
      [ "Kernel Concepts", "concepts.html", null ],
      [ "TODO", "todo.html", null ]
    ] ],
    [ "Modules", "modules.html", [
      [ "Nil", "group__nil.html", null ],
      [ "AVR core", "group___a_v_r___c_o_r_e.html", null ],
      [ "NilFIFO", "group___f_i_f_o.html", null ],
      [ "Arduino", "group__arduino.html", null ],
      [ "NilSerial", "group___serial.html", null ],
      [ "NilAnalog", "group___analog.html", null ],
      [ "NilTimer1", "group___timer1.html", null ],
      [ "TwiMaster", "group__two_wire.html", null ],
      [ "Config", "group__config.html", null ],
      [ "AVR_TYPES", "group___a_v_r___t_y_p_e_s.html", null ]
    ] ],
    [ "Data Structures", "annotated.html", [
      [ "nil_system_t", "structnil__system__t.html", null ],
      [ "nil_thread", "structnil__thread.html", null ],
      [ "nil_thread_cfg", "structnil__thread__cfg.html", null ],
      [ "NilFIFO< Type, Size >", "class_nil_f_i_f_o.html", null ],
      [ "NilSerialClass", "class_nil_serial_class.html", null ],
      [ "NilStatsFIFO< Type, Size >", "class_nil_stats_f_i_f_o.html", null ],
      [ "port_extctx", "structport__extctx.html", null ],
      [ "port_intctx", "structport__intctx.html", null ],
      [ "semaphore_t", "structsemaphore__t.html", null ],
      [ "TwiMaster", "class_twi_master.html", null ]
    ] ],
    [ "Data Structure Index", "classes.html", null ],
    [ "Data Fields", "functions.html", null ],
    [ "File List", "files.html", [
      [ "avr_heap.h", null, null ],
      [ "board.c", "board_8c.html", null ],
      [ "I2cConstants.h", "_i2c_constants_8h.html", null ],
      [ "nil.c", "nil_8c.html", null ],
      [ "nil.h", "nil_8h.html", null ],
      [ "NilAnalog.c", "_nil_analog_8c.html", null ],
      [ "NilAnalog.h", "_nil_analog_8h.html", null ],
      [ "nilconf.h", "nilconf_8h.html", null ],
      [ "nilcore.c", "nilcore_8c.html", null ],
      [ "nilcore.h", "nilcore_8h.html", null ],
      [ "NilFIFO.h", "_nil_f_i_f_o_8h.html", null ],
      [ "NilRTOS.c", "_nil_r_t_o_s_8c.html", null ],
      [ "NilRTOS.h", "_nil_r_t_o_s_8h.html", null ],
      [ "NilSerial.cpp", "_nil_serial_8cpp.html", null ],
      [ "NilSerial.h", "_nil_serial_8h.html", null ],
      [ "NilTimer1.c", "_nil_timer1_8c.html", null ],
      [ "NilTimer1.h", "_nil_timer1_8h.html", null ],
      [ "NilTwiWaitSignal.cpp", null, null ],
      [ "niltypes.h", "niltypes_8h.html", null ],
      [ "nilUtility.cpp", "nil_utility_8cpp.html", null ],
      [ "TwiMaster.h", "_twi_master_8h.html", null ],
      [ "TwiState.h", "_twi_state_8h.html", null ]
    ] ],
    [ "Globals", "globals.html", null ]
  ] ]
];

function createIndent(o,domNode,node,level)
{
  if (node.parentNode && node.parentNode.parentNode)
  {
    createIndent(o,domNode,node.parentNode,level+1);
  }
  var imgNode = document.createElement("img");
  if (level==0 && node.childrenData)
  {
    node.plus_img = imgNode;
    node.expandToggle = document.createElement("a");
    node.expandToggle.href = "javascript:void(0)";
    node.expandToggle.onclick = function() 
    {
      if (node.expanded) 
      {
        $(node.getChildrenUL()).slideUp("fast");
        if (node.isLast)
        {
          node.plus_img.src = node.relpath+"ftv2plastnode.png";
        }
        else
        {
          node.plus_img.src = node.relpath+"ftv2pnode.png";
        }
        node.expanded = false;
      } 
      else 
      {
        expandNode(o, node, false);
      }
    }
    node.expandToggle.appendChild(imgNode);
    domNode.appendChild(node.expandToggle);
  }
  else
  {
    domNode.appendChild(imgNode);
  }
  if (level==0)
  {
    if (node.isLast)
    {
      if (node.childrenData)
      {
        imgNode.src = node.relpath+"ftv2plastnode.png";
      }
      else
      {
        imgNode.src = node.relpath+"ftv2lastnode.png";
        domNode.appendChild(imgNode);
      }
    }
    else
    {
      if (node.childrenData)
      {
        imgNode.src = node.relpath+"ftv2pnode.png";
      }
      else
      {
        imgNode.src = node.relpath+"ftv2node.png";
        domNode.appendChild(imgNode);
      }
    }
  }
  else
  {
    if (node.isLast)
    {
      imgNode.src = node.relpath+"ftv2blank.png";
    }
    else
    {
      imgNode.src = node.relpath+"ftv2vertline.png";
    }
  }
  imgNode.border = "0";
}

function newNode(o, po, text, link, childrenData, lastNode)
{
  var node = new Object();
  node.children = Array();
  node.childrenData = childrenData;
  node.depth = po.depth + 1;
  node.relpath = po.relpath;
  node.isLast = lastNode;

  node.li = document.createElement("li");
  po.getChildrenUL().appendChild(node.li);
  node.parentNode = po;

  node.itemDiv = document.createElement("div");
  node.itemDiv.className = "item";

  node.labelSpan = document.createElement("span");
  node.labelSpan.className = "label";

  createIndent(o,node.itemDiv,node,0);
  node.itemDiv.appendChild(node.labelSpan);
  node.li.appendChild(node.itemDiv);

  var a = document.createElement("a");
  node.labelSpan.appendChild(a);
  node.label = document.createTextNode(text);
  a.appendChild(node.label);
  if (link) 
  {
    a.href = node.relpath+link;
  } 
  else 
  {
    if (childrenData != null) 
    {
      a.className = "nolink";
      a.href = "javascript:void(0)";
      a.onclick = node.expandToggle.onclick;
      node.expanded = false;
    }
  }

  node.childrenUL = null;
  node.getChildrenUL = function() 
  {
    if (!node.childrenUL) 
    {
      node.childrenUL = document.createElement("ul");
      node.childrenUL.className = "children_ul";
      node.childrenUL.style.display = "none";
      node.li.appendChild(node.childrenUL);
    }
    return node.childrenUL;
  };

  return node;
}

function showRoot()
{
  var headerHeight = $("#top").height();
  var footerHeight = $("#nav-path").height();
  var windowHeight = $(window).height() - headerHeight - footerHeight;
  navtree.scrollTo('#selected',0,{offset:-windowHeight/2});
}

function expandNode(o, node, imm)
{
  if (node.childrenData && !node.expanded) 
  {
    if (!node.childrenVisited) 
    {
      getNode(o, node);
    }
    if (imm)
    {
      $(node.getChildrenUL()).show();
    } 
    else 
    {
      $(node.getChildrenUL()).slideDown("fast",showRoot);
    }
    if (node.isLast)
    {
      node.plus_img.src = node.relpath+"ftv2mlastnode.png";
    }
    else
    {
      node.plus_img.src = node.relpath+"ftv2mnode.png";
    }
    node.expanded = true;
  }
}

function getNode(o, po)
{
  po.childrenVisited = true;
  var l = po.childrenData.length-1;
  for (var i in po.childrenData) 
  {
    var nodeData = po.childrenData[i];
    po.children[i] = newNode(o, po, nodeData[0], nodeData[1], nodeData[2],
        i==l);
  }
}

function findNavTreePage(url, data)
{
  var nodes = data;
  var result = null;
  for (var i in nodes) 
  {
    var d = nodes[i];
    if (d[1] == url) 
    {
      return new Array(i);
    }
    else if (d[2] != null) // array of children
    {
      result = findNavTreePage(url, d[2]);
      if (result != null) 
      {
        return (new Array(i).concat(result));
      }
    }
  }
  return null;
}

function initNavTree(toroot,relpath)
{
  var o = new Object();
  o.toroot = toroot;
  o.node = new Object();
  o.node.li = document.getElementById("nav-tree-contents");
  o.node.childrenData = NAVTREE;
  o.node.children = new Array();
  o.node.childrenUL = document.createElement("ul");
  o.node.getChildrenUL = function() { return o.node.childrenUL; };
  o.node.li.appendChild(o.node.childrenUL);
  o.node.depth = 0;
  o.node.relpath = relpath;

  getNode(o, o.node);

  o.breadcrumbs = findNavTreePage(toroot, NAVTREE);
  if (o.breadcrumbs == null)
  {
    o.breadcrumbs = findNavTreePage("index.html",NAVTREE);
  }
  if (o.breadcrumbs != null && o.breadcrumbs.length>0)
  {
    var p = o.node;
    for (var i in o.breadcrumbs) 
    {
      var j = o.breadcrumbs[i];
      p = p.children[j];
      expandNode(o,p,true);
    }
    p.itemDiv.className = p.itemDiv.className + " selected";
    p.itemDiv.id = "selected";
    $(window).load(showRoot);
  }
}

