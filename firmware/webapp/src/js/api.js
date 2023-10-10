/*
 * GET  /api/v1/status
 * GET  /api/v1/config
 * GET  /api/v1/set
 * POST /api/v1/set
 * GET  /api/v1/leds
 * GET  /api/v1/tasks
 * GET  /api/v1/animations
 * GET  /api/v1/triggerOta (?url=)
 * GET  /api/v1/ota
 * GET  /api/v1/reboot
 */

const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

export class ClockApi {
    onStatusChange;
    onConfigChange;
    onLedsChange;
    onTasksChange;
    onAnimationsChange;
    onOtaStatusChange;
    fetchInterval;

    constructor(isDev) {
        this.isDev = isDev;
        this.apiBase = isDev ? 'http://192.168.12.230/api/v1' : '/api/v1';

        // state
        this.status = null;
        this.config = null;
        this.leds = null;
        this.tasks = null;
        this.animations = null;
        this.otastatus = null;

        // interval
        this.fetchInterval = null;

        // callbacks
        this.onStatusChange = null;
        this.onConfigChange = null;
        this.onLedsChange = null;
        this.onTasksChange = null;
        this.onAnimationsChange = null;
        this.onOtaStatusChange = null;

        this.fetching = false;

        this.init();
    }

    on(event, callback) {
        switch (event) {
            case 'onStatusChange':
            case 'onConfigChange':
            case 'onLedsChange':
            case 'onTasksChange':
            case 'onAnimationsChange':
            case 'onOtaStatusChange':
                this[`${event}`] = callback;
                break;
            default:
                throw new Error(`Unknown event: ${event}`);
        }
    }

    off(event) {
        switch (event) {
            case 'onStatusChange':
            case 'onConfigChange':
            case 'onLedsChange':
            case 'onTasksChange':
            case 'onAnimationsChange':
            case 'onOtaStatusChange':
                this[`${event}`] = null;
                break;
            default:
                throw new Error(`Unknown event: ${event}`);
        }
    }

    async fetchAll() {
        if (this.fetching)
            return;

        this.fetching = true;

        // do not execute all at the same, have ~50ms delay between each
        const ms = 200;

        try {
            await this.fetchStatus();
            await delay(ms);
            await this.fetchConfig();
            await delay(ms);
            await this.fetchLeds();
            await delay(ms);
            await this.fetchTasks();
            await delay(ms);
            await this.fetchAnimations();
            await delay(ms);
            await this.fetchOtaStatus();
        } catch (e) {
            this.fetching = false;
            throw e;
        }

        this.fetching = false;
    }

    init() {
        this.status = null;
        this.config = null;
        this.leds = null;
        this.tasks = null;
        this.animations = null;
        this.otastatus = null;

        this.fetchAll();

        this.fetchInterval = setInterval(() => {
            this.fetchAll();
        }, 5000);
    }

    async fetchStatus() {
        const response = await fetch(`${this.apiBase}/status`);
        this.status = await response.json();
        if (this.onStatusChange)
            this.onStatusChange(this.status);
        return this.status;
    }

    async fetchConfig() {
        const response = await fetch(`${this.apiBase}/config`);
        this.config = await response.json();
        if (this.onConfigChange)
            this.onConfigChange(this.config);
        return this.config;
    }

    async fetchLeds() {
            const response = await fetch(`${this.apiBase}/leds`);
        this.leds = await response.json();
        if (this.onLedsChange)
            this.onLedsChange(this.leds);
        return this.leds;
    }

    async fetchTasks() {
            const response = await fetch(`${this.apiBase}/tasks`);
        this.tasks = await response.json();
        if (this.onTasksChange)
            this.onTasksChange(this.tasks);
        return this.tasks;
    }

    async fetchAnimations() {
            const response = await fetch(`${this.apiBase}/animations`);
        this.animations = await response.json();
        if (this.onAnimationsChange)
            this.onAnimationsChange(this.animations);
        return this.animations;
    }

    async fetchOtaStatus() {
            const response = await fetch(`${this.apiBase}/ota`);
        this.otastatus = await response.json();
        if (this.onOtaStatusChange)
            this.onOtaStatusChange(this.otastatus);
        return this.otastatus;
    }

    async setConfig(config) {
        const response = await fetch(`${this.apiBase}/set?${this.serializeQueryString(config)}`);
        return await response.json();
    }

    async reboot() {
        const response = await fetch(`${this.apiBase}/reboot`);
        return await response.json();
    }

    async triggerOta(url) {
        const response = await fetch(`${this.apiBase}/triggerOta?url=${encodeURIComponent(url)}`);
        return await response.json();
    }

    serializeQueryString(obj) {
        return Object.keys(obj).map(key => `${encodeURIComponent(key)}=${encodeURIComponent(obj[key])}`).join('&');
    }
}

