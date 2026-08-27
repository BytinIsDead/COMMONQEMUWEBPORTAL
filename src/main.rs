// Extreme QEMU Web Manager — AGPL-3.0-only
// Copyright (C) 2026 Extreme QEMU Web Manager contributors.
// Licensed under the GNU Affero General Public License version 3 only.
// See LICENSE for the complete license text.

mod qemu;
mod schema;

use axum::{extract::{Path, State, WebSocketUpgrade}, response::IntoResponse, routing::{get, post}, Json, Router};
use qemu::{MachineManager, QmpCommand};
use schema::VmConfig;
use std::{net::SocketAddr, sync::Arc};
use tower_http::services::ServeDir;
use tracing::info;

#[derive(Clone)]
struct AppState { manager: Arc<MachineManager> }

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::fmt().with_env_filter("info").init();
    let state = AppState { manager: Arc::new(MachineManager::default()) };
    let app = Router::new()
        .route("/api/v1/health", get(|| async { Json(serde_json::json!({"status":"ok"})) }))
        .route("/api/v1/machines", get(list_machines).post(create_machine))
        .route("/api/v1/machines/{id}/start", post(start_machine))
        .route("/api/v1/machines/{id}/stop", post(stop_machine))
        .route("/api/v1/machines/{id}/qmp", post(qmp_command))
        .route("/api/v1/machines/{id}/events", get(machine_events))
        .nest_service("/", ServeDir::new("frontend/dist").append_index_html_on_directories(true))
        .with_state(state);
    let addr: SocketAddr = "0.0.0.0:8080".parse()?;
    info!(%addr, "qemu web manager listening");
    let listener = tokio::net::TcpListener::bind(addr).await?;
    axum::serve(listener, app).await?;
    Ok(())
}

async fn list_machines(State(state): State<AppState>) -> Json<Vec<VmConfig>> { Json(state.manager.list()) }

async fn create_machine(State(state): State<AppState>, Json(config): Json<VmConfig>) -> impl IntoResponse {
    match state.manager.create(config) { Ok(vm) => (axum::http::StatusCode::CREATED, Json(vm)), Err(e) => (axum::http::StatusCode::BAD_REQUEST, Json(serde_json::json!({"error": e.to_string()}))) }
}

async fn start_machine(State(state): State<AppState>, Path(id): Path<String>) -> impl IntoResponse {
    match state.manager.start(&id).await { Ok(()) => axum::http::StatusCode::NO_CONTENT, Err(_) => axum::http::StatusCode::BAD_REQUEST }
}

async fn stop_machine(State(state): State<AppState>, Path(id): Path<String>) -> impl IntoResponse {
    match state.manager.stop(&id).await { Ok(()) => axum::http::StatusCode::NO_CONTENT, Err(_) => axum::http::StatusCode::BAD_REQUEST }
}

async fn qmp_command(State(state): State<AppState>, Path(id): Path<String>, Json(command): Json<QmpCommand>) -> impl IntoResponse {
    match state.manager.qmp(&id, command).await { Ok(result) => (axum::http::StatusCode::OK, Json(result)), Err(e) => (axum::http::StatusCode::BAD_REQUEST, Json(serde_json::json!({"error": e.to_string()}))) }
}

async fn machine_events(ws: WebSocketUpgrade, State(state): State<AppState>, Path(id): Path<String>) -> impl IntoResponse {
    ws.on_upgrade(move |mut socket| async move {
        let mut rx = state.manager.subscribe(&id);
        while let Ok(event) = rx.recv().await { if socket.send(axum::extract::ws::Message::Text(event.to_string().into())).await.is_err() { break; } }
    })
}
