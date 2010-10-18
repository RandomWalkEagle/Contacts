			(window.onresize = function() {
				
				var txt,
					wrapper,
					defWidth=0,
					txtWidth=0,
					scrollH = document.body.scrollHeight,
					j,
					i;
				    
				    function hasClass(name, type){
					    var r = [];
					    var re = new RegExp("(^|\\s)" + name + "(\\s|$)");
					    var e = document.getElementsByTagName(type || "*");
						for (var j=0; j<e.length; j++)
						    if(re.test(e[j].className)) r.push(e[j])
					    return r;
				    }
				    
				    // ������� ������������ CSS ����� ��������
				    function getStyle( elem, name ) {
					    // ���� ����� ������������ � css
					    if(elem.style[name])
						    return elem.style[name];
					    
					    // ���� ����� �� ������������ (��� ie)
					    else if (elem.currentStyle)
						    return elem.currentStyle[name];
					    
					    // W3C �����
					    else if (document.defaultView && document.defaultView.getComputedStyle) {
						    // ������ "����������" ������� �� ������, �������� textAlign �� text-align
						    name = name.replace(/([A-Z])/g,"-$1");
			    
						    name = name.toLowerCase();
						    
						    var s = document.defaultView.getComputedStyle(elem, "");
						    return s && s.getPropertyValue(name);
					    } else {
						    return null;
					    }
					    
				    }
				    
				    
				    wrapper = hasClass("message", "div");
				    txt = hasClass("txt", "span");
				    defWidth = parseInt(getStyle(wrapper[0], "width"));
				    
				    
					function countWrapper() {
						for(j=0; j<wrapper.length; j++) {
								return countWp = wrapper[j];
							}
					}
					
				    for(i=0; i<txt.length; i++) {
					    
					    if(parseInt(getStyle(txt[i], "width")) > defWidth) {
						    txt[i].style.width = defWidth -90 + "px";
					    } if(scrollH && parseInt(getStyle(txt[i], "width")) > defWidth ) {
							countWp.style.width = defWidth -20 + "px";
							txt[i].style.width = defWidth -110 + "px";
						} else {
							txt[i].style.width = defWidth -90 + "px";
						}
				    }
			})();