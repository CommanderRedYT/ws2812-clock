import { ClockApi } from './api.js';
import {handleClearConfig, handleConfigChange} from "./configExplorer.js";

window.addEventListener('load', () => {
    const isDev = window.location.hostname === 'localhost';
    window.clockApi = new ClockApi(isDev);

    window.clockApi.on('onStatusChange', status => {
        console.log('onStatusChange', status);
        document.getElementById('status').innerText = JSON.stringify(status, null, 2);
    });

    window.clockApi.on('onOtaStatusChange', animations => {
        console.log('onOtaStatusChange', animations);
        document.getElementById('ota').innerText = JSON.stringify(animations, null, 2);
    });

    window.clockApi.on('onConfigChange', config => {
        console.log('onConfigChange', config);

        handleConfigChange(config);
    });

    window.clockApi.on('onOffline', () => {
        handleClearConfig();
    });

    document.getElementById('switchOta').addEventListener('click', async () => {
        const result = await window.clockApi.switchOta();
        document.getElementById('switchOtaResult').innerText = JSON.stringify(result, null, 4);
    });
});
