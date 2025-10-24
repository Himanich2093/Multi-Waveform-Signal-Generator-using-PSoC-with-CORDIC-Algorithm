    function FunctionGeneratorGUI
        % Create the main figure
        fig = uifigure('Name', 'Function Generator Control', 'Position', [100 100 400 450]);
        
        % Waveform selection
        waveformLabel = uilabel(fig, 'Text', 'Waveform Type:', 'Position', [20 400 100 22]);
        waveformDropDown = uidropdown(fig, 'Items', {'Sine', 'Cosine', 'Square', 'Triangle', 'DC'}, ...
            'Position', [130 400 150 22], 'Value', 'Sine');
        
        % Frequency control
        freqLabel = uilabel(fig, 'Text', 'Frequency (Hz):', 'Position', [20 350 100 22]);
        freqSlider = uislider(fig, 'Position', [130 350 200 3], 'Limits', [1 100], 'Value', 10);
        freqEdit = uieditfield(fig, 'numeric', 'Position', [350 350 40 22], 'Value', 10, ...
            'Limits', [1 100], 'RoundFractionalValues', 'on');
        
        % Amplitude control
        ampLabel = uilabel(fig, 'Text', 'Amplitude (0-1):', 'Position', [20 300 100 22]);
        ampSlider = uislider(fig, 'Position', [130 300 200 3], 'Limits', [0 1], 'Value', 0.5);
        ampEdit = uieditfield(fig, 'numeric', 'Position', [350 300 40 22], 'Value', 0.5, ...
            'Limits', [0 1], 'RoundFractionalValues', 'on');
        
        % Phase control
        phaseLabel = uilabel(fig, 'Text', 'Phase (degrees):', 'Position', [20 250 100 22]);
        phaseSlider = uislider(fig, 'Position', [130 250 200 3], 'Limits', [0 360], 'Value', 0);
        phaseEdit = uieditfield(fig, 'numeric', 'Position', [350 250 40 22], 'Value', 0, ...
            'Limits', [0 360], 'RoundFractionalValues', 'on');
        
        % Update button
        updateButton = uibutton(fig, 'Text', 'Update Waveform', 'Position', [150 190 150 30], ...
            'ButtonPushedFcn', @(btn,event) updateWaveform());
        
        % Status label
        statusLabel = uilabel(fig, 'Text', 'Ready', 'Position', [20 150 360 22]);
        
        % Serial port object - initialize directly
        try
            serialObj = serialport('COM3', 115200);
            statusLabel.Text = 'Connected to COM7';
        catch e
            statusLabel.Text = ['Connection failed: ' e.message];
            serialObj = [];
        end
        
        % Link sliders and edit fields
        freqSlider.ValueChangingFcn = @(src,event) updateEditField(freqEdit, event.Value);
        freqEdit.ValueChangedFcn = @(src,event) updateSlider(freqSlider, src.Value);
        
        ampSlider.ValueChangingFcn = @(src,event) updateEditField(ampEdit, event.Value);
        ampEdit.ValueChangedFcn = @(src,event) updateSlider(ampSlider, src.Value);
        
        phaseSlider.ValueChangingFcn = @(src,event) updateEditField(phaseEdit, event.Value);
        phaseEdit.ValueChangedFcn = @(src,event) updateSlider(phaseSlider, src.Value);
        
        % Helper function to update edit fields during slider movement
        function updateEditField(editField, value)
            editField.Value = value;
        end
        
        % Helper function to update sliders when edit fields change
        function updateSlider(slider, value)
            slider.Value = value;
        end
        
        % Function to update waveform on PSoC
        function updateWaveform()
            if isempty(serialObj) || ~isvalid(serialObj)
                statusLabel.Text = 'Not connected to PSoC';
                tryReconnect();
                return;
            end
            
            try
                % Get current values from UI
                waveformType = find(strcmp(waveformDropDown.Items, waveformDropDown.Value)) - 1;
                frequency = round(freqEdit.Value);
                amplitude = round(ampEdit.Value * 255);
                phase = round(phaseEdit.Value / 360 * 255);
                
                % Create command packet (4 bytes)
                commandPacket = [uint8(waveformType), uint8(frequency), uint8(amplitude), uint8(phase)];
                
                % Send command packet
                write(serialObj, commandPacket, "uint8");
                
                statusLabel.Text = sprintf('Waveform updated: %s, %.1fHz, Amp %.2f, Phase %.0f°', ...
                    waveformDropDown.Value, frequency, ampEdit.Value, phaseEdit.Value);
                
            catch e
                statusLabel.Text = ['Error updating waveform: ' e.message];
                tryReconnect();
            end
        end
        
        % Try to reconnect to COM port if connection fails
        function tryReconnect()
            try
                if ~isempty(serialObj) && isvalid(serialObj)
                    delete(serialObj);
                end
                serialObj = serialport('COM9', 115200);
                statusLabel.Text = 'Reconnected to COM6';
            catch e
                statusLabel.Text = ['Reconnection failed: ' e.message];
                serialObj = [];
            end
        end
        
        % Cleanup when closing the figure
        fig.CloseRequestFcn = @(src,event) cleanup();
        
        function cleanup()
            if ~isempty(serialObj) && isvalid(serialObj)
                delete(serialObj);
            end
            delete(fig);
        end
    end