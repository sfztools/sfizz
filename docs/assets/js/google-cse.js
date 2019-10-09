---
layout: null
---
var searchCallback = function() {

	function getParameterByName(name) {
		name = name.replace(/[\[]/, "\\[").replace(/[\]]/, "\\]");
		var regex = new RegExp("[\\?&]" + name + "=([^&#]*)"),
		results = regex.exec(location.search);
		return results === null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
	}

	if (document.readyState == 'complete') {
	} else {
		google.setOnLoadCallback(function() {
			$('.searchbox').val(getParameterByName('q'));
			google.search.cse.element.render({ div: 'searchresults', tag: 'searchresults-only' });
		}, true);
	}

};

// Insert it before the CSE code snippet so that cse.js can take the script
// parameters, like parsetags, callbacks.
window.__gcse = {
	parsetags: 'explicit',
	callback: searchCallback
};

(function() {
	var cx = '{{ site.google.cse_code }}';
	var gcse = document.createElement('script');
	gcse.type = 'text/javascript';
	gcse.async = true;
	gcse.src = (document.location.protocol == 'https:' ? 'https:' : 'http:') +
		'//www.google.com/cse/cse.js?cx=' + cx;
	var s = document.getElementsByTagName('script')[0];
	s.parentNode.insertBefore(gcse, s);
})();
