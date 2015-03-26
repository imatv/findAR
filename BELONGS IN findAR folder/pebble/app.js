/**
 * findAR
 * Oh, there you AR.
 */

var UI = require('ui');
var ajax = require('ajax');
var Accel = require('ui/accel');

function init() {
  Accel.init();
  
  Accel.on('tap', function(e) {
    if (e.direction < 0)
      sendCmd('Face Scan');
    else
      sendCmd('Face Detect');
    console.log('Tap event on axis: ' + e.axis + ' and direction: ' + e.direction);
  });
}
init();

function sendCmd(cmd) {
  var _cmd = cmd.toLowerCase().trim();
  
  console.log('send cmd: ' + _cmd);
  
  ajax({
      url: 'http://dev.quasi.co/findar/in.php?cmd='+encodeURIComponent(_cmd),
      cache: false
    },
    function(data) {
      console.log('ajax response: ' + data);
    },
    function(error) {
      console.log('The ajax request failed: ' + error);
    });
  }

function printObject(o) {
  var out = '';
  for (var p in o) {
    out += p + ': ' + o[p] + '\n';
  }
  return out;
}

var main = new UI.Card({
  title: 'findAR',
  subtitle: 'Oh, there you AR.',
  body: 'Press any button.'
});

main.show();

var handler = function() {
  var modesMenuItems = [{
              title: 'Original'
            },{
              title: 'Outline'
            },{
              title: 'Grayscale'
            },{
              title: 'B/W'
            },{
              title: 'Sepia'
            },{
              title: 'Hue Scan'
            }];  
  var modesMenu = new UI.Menu({
    name: 'mode',
          sections: [{
            items: modesMenuItems
          }]});
  
  var colorMenuItems = [{
              title: '- Hue'
            },{
              title: '+ Hue'
            },{
              title: '- Saturation'
            },{
              title: '+ Saturation'
            },{
              title: '- Lightness'
            },{
              title: '+ Lightness'
            }];
  var colorMenu = new UI.Menu({
    name: 'color',
          sections: [{ // HSL
            items: colorMenuItems
          }]
        });
  
  var faceMenuItems = [{
    title: 'Face Detect' 
  },{
    title: 'Face Scan'
  }];
  var faceMenu = new UI.Menu({
    name: 'faces',
      sections: [{
        items: faceMenuItems
      }]
  });
  
  // top level menu
  var _items = [{
        title: 'Modes',
        subtitle: 'All the modes',
        card: null,
        menu: modesMenu
      }, {
        title: 'Color Picker',
        subtitle: 'Choose wisely',
        card: null,
        menu: colorMenu
      },
      {
        title: 'Faces',
        subtitle: 'Track \'em',
        card: null,
        menu: faceMenu
      },
      {
        title: 'Extra',
        subtitle: 'Misc stuff',
        card: {
          body: 'Coming soon...'
        }
      },
      {
        title: 'Credits',
        subtitle: 'Code, etc.',
        card: {
          body: 'Ethan_Hart,Alvin_Vuong,Yuki_Pan,Mike_Blix'
        }
      }
  ];
  
  modesMenu.on('select', function(ev){
    console.log(printObject(ev));
    console.log('selected item #: ' + ev.item);
    
    var it = modesMenuItems[ev.item];
    
    console.log(it.title);

    //console.log('action select: ' + _items[ev.item].title);
    sendCmd(it.title);
  });
  
  colorMenu.on('select', function(ev){
    console.log(printObject(ev));
    console.log('selected item #: ' + ev.item);
    
    var it = colorMenuItems[ev.item];
    
    console.log(it.title);
    
    sendCmd(it.title);
  });
  
  faceMenu.on('select', function(ev){
    console.log(printObject(ev));
    console.log('selected item #: ' + ev.item);
    
    var it = faceMenuItems[ev.item];
    console.log(it.title);
    
    sendCmd(it.title);
  });
  
  var menu = new UI.Menu({
    sections: [{
      items: _items
    }]
  });
  menu.on('select', function(e) {
    console.log('Selected item: ' + e.section + ' ' + e.item);
    
    var item = _items[e.item];
    
    if (item.menu !== undefined) {
      console.log('show menu');
      
      var _menu = item.menu;
      
      /*_menu.on('select', function(ev){
        console.log(printObject(ev));
        
        var _items = _menu.items;
        
        console.log('action select: ' + _items[ev.item].title);
        // ajax();
      });*/
      
      _menu.show();
      
    } else { // card
      console.log(item.card);
  
      var card = new UI.Card();
      card.title(item.title);
      card.subtitle(item.subtitle);
      card.body(item.card.body);
      card.show();
    }
    
  });
  
  menu.show();
};

main.on('click', 'up', handler);
main.on('click', 'select', handler);
main.on('click', 'down', handler);
