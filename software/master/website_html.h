#ifndef website_html_h
#define website_html_h

// Web server interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>FM Radio Selector</title>

  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f4f8;
      color: #333;
      margin: 0;
      padding: 20px;
      text-align: center;
    }

    h2 {
      color: #0078d7;
      font-size: 28px;
      margin-bottom: 20px;
      text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.2);
    }

    label {
      font-size: 18px;
      font-weight: bold;
      color: #0057a7;
      margin-right: 10px;
    }

    select, button {
      font-size: 16px;
      padding: 10px;
      margin: 10px 0;
      border: 2px solid #0078d7;
      border-radius: 5px;
      transition: all 0.3s ease;
    }

    select:hover, input[type="range"]:hover, button:hover {
      border-color: #0057a7;
      cursor: pointer;
    }

    input[type="range"] {
      width: 300px;
      height: 20px;
      background-color: #e0e0e0;
      border-radius: 5px;
      appearance: none;
      outline: none;
      transition: background-color 0.3s ease;
    }

    input[type="range"]::-webkit-slider-thumb {
      appearance: none;
      width: 20px;
      height: 40px;
      background: #0078d7;
      border-radius: 50%;
      cursor: pointer;
      transition: background 0.3s ease;
    }

    input[type="range"]::-webkit-slider-thumb:hover {
      background: #0057a7;
    }

    .status {
      font-size: 18px;
      margin-top: 20px;
      font-weight: bold;
      line-height: 1.5;
      color: #333;
      padding: 10px;
      background-color: #f9f9f9;
      border-radius: 10px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      display: inline-block;
    }

    .tuner-container, .slider-container, .button-container {
      margin-bottom: 30px;
    }

    button {
      background-color: #0078d7;
      color: white;
      border: none;
      padding: 12px 16px;
      font-size: 16px;
      border-radius: 5px;
      transition: background-color 0.3s ease;
    }

    button:hover {
      background-color: #0057a7;
    }

    .tuner-container {
      display: flex;
      justify-content: center;
      align-items: center;
    }
    .digit-container {
      text-align: center;
      margin: 0 5px;
    }
    .digit {
      font-family: "Courier";
      font-size: 60px;
    }

    .unclickable {
      pointer-events: none;
      cursor: default;
      opacity: 0.5;
    }

    /* Media query for smaller screens */
    @media (max-width: 600px) {
      input[type="range"] {
        width: 100%;
      }

      h2 {
        font-size: 24px;
      }

      label {
        font-size: 16px;
      }
    }
  </style>

  <script>
    // Updates webpage to follow radio if radio is ready
    function updateWebpage() {
      fetch('/status')
      .then(response => response.json())
      .then(data => {
        // Only run if radio is ready
        if(data.status == "1") {
          fetch('/update')
          .then(response => response.json())
          .then(data => {
            enableButton();

            var freqString = data.frequency; // Sending 922 instead of 92.2
            var volString = data.volume;
            var radiotext = data.radiotext;

            // Update frequency
            if(freqString.length == 3) {
              freqString = "0" + freqString;
              document.getElementById("digit1").style.opacity = 0.25;
            }
            else {
              document.getElementById("digit1").style.opacity = 1;
            }
            document.getElementById("digit1").textContent = freqString.charAt(0);
            document.getElementById("digit2").textContent = freqString.charAt(1);
            document.getElementById("digit3").textContent = freqString.charAt(2);
            document.getElementById("digit4").textContent = freqString.charAt(3);

            // Update display buttons
            updateButton();

            // Update volume
            document.getElementById("volume").value = parseInt(volString);
            document.getElementById("volDisplay").innerHTML = volString;

            // Bottom status update
            document.getElementById("freq-status").innerHTML = "Selected frequency: " + String((parseInt(freqString)/10).toFixed(1)) + "MHz";
            document.getElementById("vol-status").innerHTML = "Volume: " + volString;
            document.getElementById("radiotext-status").innerHTML = "RDS: " + radiotext;
          });
        }
        else {
          disableButton();

          // Update the status dynamically
          document.getElementById("freq-status").innerHTML = "Tuning...";
          document.getElementById("vol-status").innerHTML = "";
          document.getElementById("radiotext-status").innerHTML = "";
        }
      });
    }

    // Update the values every 50ms
    var refreshIntervalId = setInterval(updateWebpage, 100);

    function readFrequency() {
      var frequency = 0;
      var digitVal = 100;
      frequency += parseInt(document.getElementById("digit1").textContent) * 100;
      frequency += parseInt(document.getElementById("digit2").textContent) * 10;
      frequency += parseInt(document.getElementById("digit3").textContent);
      frequency += parseInt(document.getElementById("digit4").textContent) * 0.1;

      return frequency;
    }

    function changeFrequency(digitValue, direction) {
      disableButton();
      clearInterval(refreshIntervalId);
      // Update the status dynamically
      document.getElementById("freq-status").innerHTML = "Tuning...";

      var frequency = readFrequency();
      var volume = document.getElementById("volume").value;

      if(direction == 'up') {
        frequency += parseFloat(digitValue);
      }
      else if(direction == 'down') {
        frequency -= parseFloat(digitValue);
      }

      if(frequency < 87.0 || frequency > 108.0) {
        return;
      }

      // Send the volume values to the server, and also frequency together
      fetch("/get?frequency=" + frequency + "&volume=" + volume)
      .then(response => response.text())
      .catch(error => console.error("Error:", error));

      // Resume refresh when radio is ready
      refreshIntervalId = setInterval(updateWebpage, 100);
    }

    // Update whether buttons are displayed
    function updateButton() {
      var frequency = readFrequency();
      var min = 87.0; var max = 108.0;
      
      if(frequency + 100 > max) document.getElementById("button_1up").style.visibility="hidden";
      else document.getElementById("button_1up").style.visibility="visible";
      if(frequency - 100 < min) document.getElementById("button_1down").style.visibility="hidden";
      else document.getElementById("button_1down").style.visibility="visible";

      if(frequency + 10 > max) document.getElementById("button_2up").style.visibility="hidden";
      else document.getElementById("button_2up").style.visibility="visible";
      if(frequency - 10 < min) document.getElementById("button_2down").style.visibility="hidden";
      else document.getElementById("button_2down").style.visibility="visible";

      if(frequency + 1 > max) document.getElementById("button_3up").style.visibility="hidden";
      else document.getElementById("button_3up").style.visibility="visible";
      if(frequency - 1 < min) document.getElementById("button_3down").style.visibility="hidden";
      else document.getElementById("button_3down").style.visibility="visible";

      if(frequency + 0.1 > max) document.getElementById("button_4up").style.visibility="hidden";
      else document.getElementById("button_4up").style.visibility="visible";
      if(frequency - 0.1 < min) document.getElementById("button_4down").style.visibility="hidden";
      else document.getElementById("button_4down").style.visibility="visible";
    }

    // Disable all buttons, volume too
    function disableButton() {
      document.getElementById("button_1up").classList.add('unclickable');
      document.getElementById("button_1down").classList.add('unclickable');
      document.getElementById("button_2up").classList.add('unclickable');
      document.getElementById("button_2down").classList.add('unclickable');
      document.getElementById("button_3up").classList.add('unclickable');
      document.getElementById("button_3down").classList.add('unclickable');
      document.getElementById("button_4up").classList.add('unclickable');
      document.getElementById("button_4down").classList.add('unclickable');

      document.getElementById("volume").classList.add('unclickable');

      document.getElementById("tuning-buttons").classList.add('unclickable');
    }

    // Enable back buttons
    function enableButton() {
      document.getElementById("button_1up").classList.remove('unclickable');
      document.getElementById("button_1down").classList.remove('unclickable');
      document.getElementById("button_2up").classList.remove('unclickable');
      document.getElementById("button_2down").classList.remove('unclickable');
      document.getElementById("button_3up").classList.remove('unclickable');
      document.getElementById("button_3down").classList.remove('unclickable');
      document.getElementById("button_4up").classList.remove('unclickable');
      document.getElementById("button_4down").classList.remove('unclickable');

      document.getElementById("volume").classList.remove('unclickable');

      document.getElementById("tuning-buttons").classList.remove('unclickable');
    }

    function changeVolume() {
      disableButton();
      clearInterval(refreshIntervalId);
      // Update the status dynamically
      document.getElementById("vol-status").innerHTML = "Tuning...";

      var volume = document.getElementById("volume").value;
      var frequency = readFrequency();

      // Send the volume values to the server, and also frequency together
      fetch("/get?frequency=" + frequency + "&volume=" + volume)
      .then(response => response.text())
      .catch(error => console.error("Error:", error));

      // Resume refresh when radio is ready
      refreshIntervalId = setInterval(updateWebpage, 100);
    }

    function changeVolumeDisplay() {
      if (typeof refreshIntervalId !== 'undefined' && refreshIntervalId !== null) {
        clearInterval(refreshIntervalId);
        refreshIntervalId = null;
      }

      var volume = document.getElementById("volume").value;

      // Update value beside slider
      document.getElementById("volDisplay").innerHTML = volume;
    }

    function tuneFrequency(direction) {
      disableButton();
      clearInterval(refreshIntervalId);
      // Update the status dynamically
      document.getElementById("freq-status").innerHTML = "Tuning...";
      document.getElementById("vol-status").innerHTML = "Tuning...";

      // Send the tune command to the server
      fetch("/get?tune=" + direction)
      .then(response => response.text())
      .catch(error => console.error("Error:", error));

      // Resume refresh when radio is ready
      refreshIntervalId = setInterval(updateWebpage, 100);
    }
  </script>
</head>

<body onload="updateButton()">
  <h2>FM Radio Selector</h2>

  <div class="tuner-container">
    <div class="digit-container">
      <button id="button_1up" class="button" onclick="changeFrequency('100', 'up')">&#9650</button>
      <div id="digit1" class="digit" style="opacity: 0.25;">0</div>
      <button id="button_1down" class="button" onclick="changeFrequency('100', 'down')">&#9660</button>
    </div>

    <div class="digit-container">
      <button id="button_2up" class="button" onclick="changeFrequency('10', 'up')">&#9650</button>
      <div id="digit2" class="digit">8</div>
      <button id="button_2down" class="button" onclick="changeFrequency('10', 'down')">&#9660</button>
    </div>
    
    <div class="digit-container">
      <button id="button_3up" class="button" onclick="changeFrequency('1', 'up')">&#9650</button>
      <div id="digit3" class="digit">7</div>
      <button id="button_3down" class="button" onclick="changeFrequency('1', 'down')">&#9660</button>
    </div>
    
    <div class="digit-container">
      <div id="non-digit" class="digit" style="margin: 0 -15px;">.</div>
    </div>
    
    <div class="digit-container">
      <button id="button_4up" class="button" onclick="changeFrequency('0.1', 'up')">&#9650</button>
      <div id="digit4" class="digit">0</div>
      <button id="button_4down" class="button" onclick="changeFrequency('0.1', 'down')">&#9660</button>
    </div>

    <div class="digit-container"></div>
      <div id="non-digit" class="digit">MHz</div>
    </div>
  </div>

  <div class="slider-container">
    <label for="volume">VOLUME</label>
    <input type="range" id="volume" name="volume" min="0" max="15" step="1" value="0" class="slider" oninput="changeVolumeDisplay()" onchange="changeVolume()">
    <span id="volDisplay" style="font-size: 20px; font-weight: bold;">0</span>
  </div>

  <!-- Buttons for tuning frequency up and down -->
  <div id="tuning-buttons" class="button-container">
    <button onclick="tuneFrequency('down')">&#9660 TUNE DOWN</button>
    <button onclick="tuneFrequency('up')">TUNE UP &#9650</button>
  </div>

  <div class="status">
    <div id="freq-status">Selected frequency: 87.0MHz</div>
    <div id="vol-status">Volume: 0</div>
    <div id="radiotext-status">RDS: </div>
  </div>

</body>
</html>
)rawliteral";

#endif