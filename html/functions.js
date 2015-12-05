
function setAC(room)
{
    var mode = "";
    if ($('#' + room + '-mode-heat').prop('checked')) mode = "heat";
    if ($('#' + room + '-mode-cool').prop('checked')) mode = "cool";

    var fan = "";
    if ($('#' + room + '-fan-1').prop('checked')) fan = "1";
    if ($('#' + room + '-fan-2').prop('checked')) fan = "2";
    if ($('#' + room + '-fan-3').prop('checked')) fan = "3";
    if ($('#' + room + '-fan-auto').prop('checked')) fan = "auto";

    $.ajax({
		type: "GET",
		url: "save.cgi",
		data: {
	    power: $('#' + room + '-power').val(),
	    mode: mode,
	    temp: $('#' + room + '-temp').text(),
	    fan: fan,
	    swing: $('#' + room + '-swing').val()
	}
    }).done(function(msg) {
		if (msg != "OK") alert('error');
    });
}

function load()
{
	$.ajax({
		type: "GET",
		url: "settings.cgi"
    }).done(function(conf) {
		conf = jQuery.parseJSON(conf);
		$('#service-power').val(conf.power).slider("refresh");
		$('input:radio').each(function(index) {
	    	if (conf.mode == this.id.substring(13))
				$('#' + this.id).prop('checked', true).checkboxradio("refresh");
		});
		$('#service-temp').text(conf.temp);
		$('input:radio').each(function(index) {
	    	if (conf.fan == this.id.substring(12))
				$('#' + this.id).prop('checked', true).checkboxradio("refresh");
		});
		$('#service-swing').val(conf.swing).slider("refresh");
		$('#temp_humid').text(parseFloat((conf.dht_temp / 100).toFixed(2)) + "C, " + parseFloat((conf.dht_humid / 100).toFixed(2)) + "%");

		setTimeout(function() {
			load();
		}, 60000);
    });
}

$(document).ready(function() {
    load();

    $('#service-power').bind("change", function(event, ui) { setAC('service'); });
    $('#service-mode-heat').bind("change", function(event, ui) { setAC('service'); });
    $('#service-mode-cool').bind("change", function(event, ui) { setAC('service'); });
    $('#service-temp-down').on('vclick', function() {
		var temp = parseInt($('#service-temp').text()) - 1;
		if (temp < 15) temp = 15;
		$('#service-temp').text(temp);
		setAC('service'); 
    });
    $('#service-temp-up').on('vclick', function() {
		var temp = parseInt($('#service-temp').text()) + 1;
		if (temp > 30) temp = 30;
		$('#service-temp').text(temp);
		setAC('service'); 
    });
    $('#service-fan-1').bind("change", function(event, ui) { setAC('service'); });
    $('#service-fan-2').bind("change", function(event, ui) { setAC('service'); });
    $('#service-fan-3').bind("change", function(event, ui) { setAC('service'); });
    $('#service-fan-auto').bind("change", function(event, ui) { setAC('service'); });
    $('#service-swing').bind("change", function(event, ui) { setAC('service'); });
});
