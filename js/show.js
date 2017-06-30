function readCvs() {
	Plotly.d3.csv("arq.csv", function(data){
		processData(data)
	});
}

function processData(allRows) {

	var csvdata = {}	
	var arqwin = ['srx','stx','etx','erx']

	for (var i=0; i<allRows.length; i++) {
		row = allRows[i];
		ts = row['ts'];
		node = row['node'];
		if (!(ts in csvdata)) {
		   csvdata[ts] = {};
		}

		for (var j = 0; j < arqwin.length; j++) {
			var key = arqwin[j];		
			if (!(key in csvdata[ts])) {
			   csvdata[ts][key] = {};
			}
			csvdata[ts][key][node] = row[key];
		}
	}

	createTraces(csvdata)

}

window.traces = {};

function createTraces(csvdata) {
	
	for (var ts in csvdata) {
		var fr = 'frame' + ts.toString();
		var xs = [];
		var ys = [];
		for (var ak in csvdata[ts])
		{
			var y = [];
			var x = [];
			for (var node in csvdata[ts][ak])		
			{
				x.push(csvdata[ts][ak][node]);
				y.push(node);
			}
			xs.push(x);
			ys.push(y);
		}
		
		traces[ts] = [];
		for (var j=0; j < xs.length; j++)
		{
			traces[ts].push({x: xs[j], y: ys[j], mode: 'markers', type: 'scatter', marker: { size: 12 }});
		}
	}	


	createFrames(traces)

}

window.frames = [];

function createFrames(traces)
{
	for(var ts in traces)
	{
		var frame = 'frame' + ts.toString();
		frames.push({data: traces[ts], name: frame});
	}
	createSteps(traces)
}

window.allSteps = [];

function createSteps(traces)
{
	for(var ts in traces)
	{
		var frame = 'frame' + ts.toString();
		allSteps.push({
		  label: ts.toString(),
		  method: 'animate',
		  args: [[frame], {
		      mode: 'immediate',
		      frame: {redraw: false, duration: 500},
		      transition: {duration: 500}
		    }]
		});
	}

}


function doPlot() {

	Plotly.plot('myDiv', Object.values(traces)[0],
 	{
	  updatemenus: [{
		direction: 'left',
        pad: {r: 10, t: 87},
        showactive: false,
        type: 'buttons',
        x: 0.1,
        xanchor: 'right',
        y: 0,
        yanchor: 'top',
        buttons: [
            {
                args: [null, {frame: {duration: 500, redraw: false},
									fromcurrent: true, 
									transition: {duration: 300, easing: 'quadratic-in-out'}
								}
						],
                label: 'Play',
                method: 'animate'
            },
			{
                args: [null, {frame: {duration: 100, redraw: false},
									fromcurrent: true, 
									transition: {duration: 50, easing: 'quadratic-in-out'}
								}
						],
                label: 'Play fast',
                method: 'animate'
            },
            {
                args: [[null], {frame: {duration: 0, redraw: false}, 
									mode: 'immediate',
						            transition: {duration: 0}
								}
						],
                label: 'Pause',
                method: 'animate'
            }
        ]        
    }]
	},
    {
	  xaxis: {autorange: true},
	  yaxis: {autorange: true}
	}).then(function () {
	  Plotly.addFrames('myDiv', frames);
	})

}

window.frame_c = 0;

function clickNext() {
 /*
	gd = helpers.getGraphDiv('myDiv');

	var currentFrame = gd._fullLayout._currentFrame;
	var nextFrame;
	if(currentFrame) {
		var idx = -1;
		for(i = 0; i < frames.length; i++) {
		    frame = frames[i];
		    if(frame.type === 'byname' && frame.name === currentFrame) {
		        idx = i;
		        break;
		    }
		}

		if(idx > 0 && idx < frameList.length - 1) {
			nextFrame = frames[idx + 1];
		}
	}
	if(nextFrame)
	{
	 	Plotly.animate('myDiv', [nextFrame], {
		  xaxis: {autorange: true},
		  yaxis: {autorange: true},
		frame: [
		  {duration: 200},
		],
		transition: [
		  {duration: 200, easing: 'cubic-in-out'},
		],
		mode: 'afterall'
		})
	}
*/
/* frame_c++
  if(frame_c==frames.length)frame_c=0
  k = Object.keys(traces)[frame_c]
  var fr = 'frame' + k.toString()
 
  Plotly.animate('myDiv', [fr], {
	  xaxis: {autorange: true},
	  yaxis: {autorange: true},
    frame: [
      {duration: 200},
    ],
    transition: [
      {duration: 200, easing: 'cubic-in-out'},
    ],
    mode: 'afterall'
  })
*/
}

function clickPrev() {

}


///////////////////////////
readCvs();
setTimeout(function(){ 

console.log(frames)
console.log(allSteps.length)


console.log(Object.values(traces)[0])

doPlot();

    }, 3000);



