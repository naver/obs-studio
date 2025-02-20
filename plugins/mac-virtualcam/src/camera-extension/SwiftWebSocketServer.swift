//
//  SwiftWebSocketServer.swift
//  SwiftSocketServer
//
//  Copyright © 2024 Neas Lease. All rights reserved.
//

import Foundation
import Network

class SwiftWebSocketServer {
	static let singleton: SwiftWebSocketServer? = {
		return (0..<3).reduce(nil) { partialResult, _ in
			partialResult ?? SwiftWebSocketServer(port: UInt16.random(in: 49152..<65535))
		}
	}()
	
#if DEBUG
	static let test_singleton: SwiftWebSocketServer? = {
		return (0..<3).reduce(nil) { partialResult, _ in
			partialResult ?? SwiftWebSocketServer(port: 59500)
		}
	}()
#endif
	
	let port: NWEndpoint.Port
	let listener: NWListener
	let parameters: NWParameters
	
	init?(port: UInt16) {
		guard let p = NWEndpoint.Port(rawValue: port) else { return nil }
		self.port = p
		
		self.parameters = {
			var paras = NWParameters(tls: nil)
			paras.allowLocalEndpointReuse = true
			paras.includePeerToPeer = true
			let wsOptions = NWProtocolWebSocket.Options()
			wsOptions.autoReplyPing = true
			paras.defaultProtocolStack.applicationProtocols.insert(wsOptions, at: 0)
			return paras
		}()
		
		guard let listener = try? NWListener(using: parameters, on: self.port) else { return nil }
		self.listener = listener
		self.listener.stateUpdateHandler = self.stateDidChange(to:)
		self.listener.newConnectionHandler = self.didAccept(nwConnection:)
		self.listener.start(queue: .main)
	}
	
	func appendExtensionClientInfo(_ eci: ExtensionClientInfo) {
		DispatchQueue.main.async { [weak self] in
			self?.exentionClientInfos.append(eci)
		}
	}
	
	private func stateDidChange(to newState: NWListener.State) {
		switch newState {
			case .ready:
				print("Server ready.")
			case .failed(let error):
				print("Server failure, error: \(error.localizedDescription)")
			default:
				break
		}
	}
	
	private func didAccept(nwConnection: NWConnection) {
		let connection = ServerConnection(nwConnection: nwConnection)
		connectionsByID[connection.id] = connection
		
		connection.start()
		
		connection.didStopCallback = { [weak self] err in
			self?.connectionDidStop(connection)
		}
		connection.didReceive = { [weak self, weak connection] data in
			guard let str = String(data: data, encoding: .utf8), let cmd = CommandType(rawValue: str), let `self` = self else {
				return
			}
			
			switch cmd {
				case .popExtensionClientInfoReq:
					let rsp = ExtensionClientInfoRsp(
						commandType: .popExtensionClientInfoRsp,
						data: exentionClientInfos)
					
					do {
						let jsonData = try JSONEncoder().encode(rsp)
						connection?.send(data: jsonData)
						exentionClientInfos.removeAll()
					} catch {
						print(error)
					}
					
				case .popExtensionClientInfoRsp:
					break
			}
			
		}
		print("server did open connection \(connection.id)")
	}
	
	private func connectionDidStop(_ connection: ServerConnection) {
		self.connectionsByID.removeValue(forKey: connection.id)
		print("server did close connection \(connection.id)")
	}
	
	private func stop() {
		self.listener.stateUpdateHandler = nil
		self.listener.newConnectionHandler = nil
		self.listener.cancel()
		for connection in self.connectionsByID.values {
			connection.didStopCallback = nil
			connection.stop()
		}
		self.connectionsByID.removeAll()
	}
	
	private var connectionsByID: [Int: ServerConnection] = [:]
	private var exentionClientInfos: [ExtensionClientInfo] = []
}

// MARK: - ServerConnection

class ServerConnection {
	private static var nextID: Int = 0
	let connection: NWConnection
	let id: Int
	
	init(nwConnection: NWConnection) {
		connection = nwConnection
		id = ServerConnection.nextID
		ServerConnection.nextID += 1
	}
	
	deinit {
		print("deinit")
	}
	
	var didStopCallback: ((Error?) -> Void)? = nil
	var didReceive: ((Data) -> ())? = nil
	
	func start() {
		print("connection \(id) will start")
		connection.stateUpdateHandler = self.stateDidChange(to:)
		setupReceive()
		connection.start(queue: .main)
	}
	
	private func stateDidChange(to state: NWConnection.State) {
		switch state {
			case .waiting(let error):
				connectionDidFail(error: error)
			case .ready:
				print("connection \(id) ready")
			case .failed(let error):
				connectionDidFail(error: error)
			default:
				break
		}
	}
	
	private func setupReceive() {
		connection.receiveMessage() { [weak self] (data, context, isComplete, error) in
			if let data = data, let context = context, !data.isEmpty {
				self?.handleMessage(data: data, context: context)
			}
			if let error = error {
				self?.connectionDidFail(error: error)
			} else {
				self?.setupReceive()
			}
		}
	}
	
	func handleMessage(data: Data, context: NWConnection.ContentContext) {
		didReceive?(data)
	}
	
	func send(data: Data) {
		let metaData = NWProtocolWebSocket.Metadata(opcode: .binary)
		let context = NWConnection.ContentContext (identifier: "context", metadata: [metaData])
		connection.send(content: data, contentContext: context, isComplete: true, completion: .contentProcessed( { [weak self] error in
			guard let self = self else { return }
			if let error = error {
				self.connectionDidFail(error: error)
				return
			}
			print("connection \(self.id) did send, data: \(data as NSData)")
		}))
	}
	
	func stop() {
		print("connection \(id) will stop")
	}
	
	private func connectionDidFail(error: Error) {
		print("connection \(id) did fail, error: \(error)")
		stop(error: error)
	}
	
	private func connectionDidEnd() {
		print("connection \(id) did end")
		stop(error: nil)
	}
	
	private func stop(error: Error?) {
		connection.stateUpdateHandler = nil
		connection.cancel()
		if let didStopCallback = didStopCallback {
			self.didStopCallback = nil
			didStopCallback(error)
		}
		didReceive = nil
	}
}

// MARK: - SwiftWebSocketServer struct & enum

extension SwiftWebSocketServer {
	enum CommandType: String, Codable {
		case popExtensionClientInfoReq = "POP_EXTENSION_CLIENT_INFO_REQ"
		case popExtensionClientInfoRsp = "POP_EXTENSION_CLIENT_INFO_RSP"
	}
	struct ExtensionClientInfo: Codable {
		let pid: Int32
		let signingId: String
		let clientId: String
		let date: String
	}
	struct ExtensionClientInfoRsp: Codable {
		let commandType: CommandType
		let data: [ExtensionClientInfo]
	}
}
